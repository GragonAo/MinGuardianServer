#include "account.h"
#include "libserver/common.h"
#include "libserver/component_help.h"
#include "libserver/log4_help.h"
#include "libserver/message_system_help.h"

#include "libplayer/player.h"
#include "libplayer/player_collector_component.h"
#include "libplayer/player_component_proto_list.h"
#include "libresource/resource_help.h"
#include "libserver/global.h"
#include "libserver/message_system.h"
#include "libserver/packet.h"
#include "libserver/protobuf/msg.pb.h"
#include "libserver/protobuf/proto_id.pb.h"
#include "libserver/socket_object.h"
#include "libserver/timer_component.h"
#include "player_component_account.h"
#include "player_component_onlineinlogin.h"
#include <queue>
#include <utility>

// Account 类的 Awake 方法，用于初始化组件、注册消息处理函数和定时器
void Account::Awake() {
  // 添加玩家收集器组件，管理玩家对象
  AddComponent<PlayerCollectorComponent>();

  // HTTP请求相关设置
  auto pYaml = ComponentHelp::GetYaml();
  const auto pLoginConfig =
      dynamic_cast<LoginConfig *>(pYaml->GetConfig(APP_LOGIN));
  ParseUrlInfo info;
  info.Mothed = pLoginConfig->UrlMethod;
  if (!MessageSystemHelp::ParseUrl(pLoginConfig->UrlLogin, info)) {
    LOG_ERROR("解析登录URL失败，URL：" << pLoginConfig->UrlLogin.c_str());
  } else {
    // 保存解析后的HTTP地址、端口和方法
    _httpIp = info.Host;
    _httpPort = info.Port;
    _method = pLoginConfig->UrlMethod;
    _loginPath = pLoginConfig->UrlLogin;
    _registerPath = pLoginConfig->UrlRegister;
  }

  // 定时器：每10秒同步应用信息到应用管理器
  AddTimer(0, 10, true, 2, BindFunP0(this, &Account::SyncAppInfoToAppMgr));

  // 获取消息系统对象
  auto pMsgSystem = GetSystemManager()->GetMessageSystem();

  // 注册消息处理函数：检查账户
  pMsgSystem->RegisterFunction(this, Proto::MsgId::C2L_AccountCheck,
                               BindFunP1(this, &Account::HandleAccountCheck));

  pMsgSystem->RegisterFunction(
      this, Proto::MsgId::C2L_AccountRegister,
      BindFunP1(this, &Account::HandleAccountRegister));

  // 注册消息处理函数：查询在线账户
  pMsgSystem->RegisterFunction(
      this, Proto::MsgId::MI_AccountQueryOnlineToRedisRs,
      BindFunP1(this, &Account::HandleAccountQueryOnlineToRedisRs));
  // 注册消息处理函数：处理HTTP响应
  pMsgSystem->RegisterFunction(
      this, Proto::MsgId::MI_HttpOuterResponse,
      BindFunP1(this, &Account::HandleHttpOuterResponse));

  // 注册消息处理函数：网络连接断开
  pMsgSystem->RegisterFunction(
      this, Proto::MsgId::MI_NetworkDisconnect,
      BindFunP1(this, &Account::HandleNetworkDisconnect));
  // 注册消息处理函数：网络连接建立
  pMsgSystem->RegisterFunction(
      this, Proto::MsgId::MI_NetworkConnected,
      BindFunP1(this, &Account::HandleNetworkConnected));

  // 注册消息处理函数：同步应用信息列表
  pMsgSystem->RegisterFunction(
      this, Proto::MsgId::MI_AppInfoListSync,
      BindFunP1(this, &Account::HandleAppInfoListSync));

  // 注册消息处理函数：处理Redis返回的登录Token
  pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_LoginTokenToRedisRs,
                               BindFunP1(this, &Account::HandleTokenToRedisRs));

  // db
  pMsgSystem->RegisterFunction(
      this, Proto::MsgId::L2DB_QueryPlayerListRs,
      BindFunP1(this, &Account::HandleQueryPlayerListRs));
  // player
  pMsgSystem->RegisterFunction(this, Proto::MsgId::C2L_CreatePlayer,
                               BindFunP1(this, &Account::HandleCreatePlayer));
  pMsgSystem->RegisterFunction(this, Proto::MsgId::L2DB_CreatePlayerRs,
                               BindFunP1(this, &Account::HandleCreatePlayerRs));
  pMsgSystem->RegisterFunction(this, Proto::MsgId::C2L_SelectPlayer,
                               BindFunP1(this, &Account::HandleSelectPlayer));
}

// 清理资源，当对象被重置或销毁时调用
void Account::BackToPool() {
  // 清空应用列表
  _apps.clear();
}

// 定时同步应用信息到应用管理器
void Account::SyncAppInfoToAppMgr() {
  Proto::AppInfoSync protoSync;
  protoSync.set_app_id(Global::GetInstance()->GetCurAppId()); // 设置当前应用ID
  protoSync.set_app_type(
      (int)Global::GetInstance()->GetCurAppType()); // 设置当前应用类型
  protoSync.set_online(GetComponent<PlayerCollectorComponent>()
                           ->OnlineSize()); // 获取在线玩家数量

  // 发送同步信息到应用管理器
  MessageSystemHelp::SendPacket(Proto::MsgId::MI_AppInfoSync, protoSync,
                                APP_APPMGR);
}

// 处理应用信息同步
void Account::HandleAppInfoListSync(Packet *pPacket) {
  // 解析从Packet中获取的Proto数据
  auto proto = pPacket->ParseToProto<Proto::AppInfoListSync>();
  for (auto one : proto.apps()) {
    Parse(one, INVALID_SOCKET); // 解析应用信息
  }
}

// 处理网络连接建立
void Account::HandleNetworkConnected(Packet *pPacket) {
  auto pTagValue = pPacket->GetTagKey()->GetTagValue(TagType::Account);
  if (pTagValue == nullptr)
    return;

  if (pPacket->GetSocketKey()->NetType != NetworkType::HttpConnector)
    return;

  auto account = pTagValue->KeyStr;
  auto pPlayerCollector = GetComponent<PlayerCollectorComponent>();

  // 根据账户查找玩家
  auto pPlayer = pPlayerCollector->GetPlayerByAccount(account);
  if (pPlayer == nullptr) {
    LOG_ERROR("HTTP连接成功，但未找到玩家，账户：" << account.c_str()
                                                   << pPacket);
    return;
  }

  const auto pPlayerCAccount = pPlayer->GetComponent<PlayerComponentAccount>();

#ifdef LOG_TRACE_COMPONENT_OPEN
  // 记录跟踪日志
  ComponentHelp::GetTraceComponent()->TraceAccount(
      pPlayer->GetAccount(), pPacket->GetSocketKey()->Socket);
#endif

  // 将玩家的Http请求分配给当前的这个Http请求连接器
  if (_httpCallbackType.count(pPacket->GetSocketKey()->Socket)) {
    _httpCallbackType.erase(pPacket->GetSocketKey()->Socket);
  }
  if (!_playerSendHttpType.count(pPlayer->GetPlayerSN()) ||
      _playerSendHttpType[pPlayer->GetPlayerSN()].empty()) {
    MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_NetworkRequestDisconnect,
                                      pPacket);
    return;
  }
  auto httpCallbackType = _playerSendHttpType[pPlayer->GetPlayerSN()].front();
  _httpCallbackType.insert(
      std::make_pair(pPacket->GetSocketKey()->Socket, httpCallbackType));
  _playerSendHttpType[pPlayer->GetPlayerSN()].pop();

  // 玩家Http请求队列资源监测
  if (_playerSendHttpType[pPlayer->GetPlayerSN()].empty()) {
    _playerSendHttpType.erase(pPlayer->GetPlayerSN());
  }

  // 发送HTTP验证请求
  NetIdentify httpIndentify;
  httpIndentify.GetSocketKey()->CopyFrom(pPacket->GetSocketKey());
  httpIndentify.GetTagKey()->CopyFrom(pPlayer->GetTagKey());

  std::map<std::string, std::string> params;
  params["account"] = pPlayer->GetAccount();           // 账户
  params["password"] = pPlayerCAccount->GetPassword(); // 密码

  // 分类型进行Http请求
  if (httpCallbackType == Proto::MsgId::C2L_AccountCheck) {
    MessageSystemHelp::SendHttpRequest(&httpIndentify, _httpIp, _httpPort,
                                       _method, _loginPath, &params);
  }
}

// 处理网络连接断开
void Account::HandleNetworkDisconnect(Packet *pPacket) {
  const auto socketKey = pPacket->GetSocketKey();
  if (socketKey->NetType != NetworkType::TcpListen) // 如果是Http断开
  {
    _httpCallbackType.erase(socketKey->Socket);
    return;
  }

  auto pPlayerCollector = GetComponent<PlayerCollectorComponent>();
  // 移除与断开Socket关联的玩家
  pPlayerCollector->RemovePlayerBySocket(pPacket->GetSocketKey()->Socket);
}

// 断开指定玩家的Socket连接
void Account::SocketDisconnect(std::string account, NetIdentify *pIdentify) {
  Proto::AccountCheckRs protoResult;
  protoResult.set_return_code(
      Proto::AccountCheckReturnCode::ARC_LOGGING); // 设置返回状态为正在登录中
  MessageSystemHelp::SendPacket(Proto::MsgId::C2L_AccountCheckRs, pIdentify,
                                protoResult); // 发送断开连接消息

  // 调度断开连接的Packet
  MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_NetworkRequestDisconnect,
                                    pIdentify);
}

// 处理账户检查
void Account::HandleAccountCheck(Packet *pPacket) {
  auto protoCheck =
      pPacket->ParseToProto<Proto::AccountCheck>(); // 解析账户检查请求Proto

  auto pPlayerCollector = GetComponent<PlayerCollectorComponent>();

  // 检查同一账户是否已登录
  auto pPlayer = pPlayerCollector->GetPlayerByAccount(protoCheck.account());
  if (pPlayer != nullptr) {
    LOG_WARN("账户检查失败，账户已登录：" << protoCheck.account().c_str()
                                          << ". " << pPlayer);
    SocketDisconnect(protoCheck.account(), pPacket); // 断开连接
    return;
  }

  // 添加玩家信息
  pPlayer = pPlayerCollector->AddPlayer(pPacket, protoCheck.account());
  if (pPlayer == nullptr) {
    LOG_WARN("账户检查失败，相同Socket存在" << pPacket);
    SocketDisconnect(protoCheck.account(), pPacket);
    return;
  }

  // 添加账户组件
  pPlayer->AddComponent<PlayerComponentAccount>(protoCheck.password());

#ifdef LOG_TRACE_COMPONENT_OPEN
  // 记录跟踪日志
  ComponentHelp::GetTraceComponent()->TraceAccount(
      protoCheck.account(), pPacket->GetSocketKey()->Socket);
#endif

  // 向Redis查询在线状态
  Proto::AccountQueryOnlineToRedis protoToRedis;
  protoToRedis.set_account(pPlayer->GetAccount().c_str());
  MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_AccountQueryOnlineToRedis,
                                    protoToRedis, nullptr);

  // 将玩家请求的Http加入到代请求队列中
  if (!_playerSendHttpType.count(pPlayer->GetPlayerSN())) {
    _playerSendHttpType[pPlayer->GetPlayerSN()] = std::queue<Proto::MsgId>();
  }
  _playerSendHttpType[pPlayer->GetPlayerSN()].push(
      Proto::MsgId::C2L_AccountCheck);
}

void Account::HandleAccountRegister(Packet *pPacket) {
  auto protoCheck =
      pPacket->ParseToProto<Proto::AccountCheck>(); // 解析账户检查请求Proto

  auto pPlayerCollector = GetComponent<PlayerCollectorComponent>();

  // 检查同一账户是否已登录
  auto pPlayer = pPlayerCollector->GetPlayerByAccount(protoCheck.account());
  if (pPlayer != nullptr) {
    LOG_WARN("账户检查失败，账户已登录：" << protoCheck.account().c_str()
                                          << ". " << pPlayer);
    SocketDisconnect(protoCheck.account(), pPacket); // 断开连接
    return;
  }

  // 添加玩家信息
  pPlayer = pPlayerCollector->AddPlayer(pPacket, protoCheck.account());
  if (pPlayer == nullptr) {
    LOG_WARN("账户检查失败，相同Socket存在" << pPacket);
    SocketDisconnect(protoCheck.account(), pPacket);
    return;
  }

  // 添加账户组件
  pPlayer->AddComponent<PlayerComponentAccount>(protoCheck.password());

#ifdef LOG_TRACE_COMPONENT_OPEN
  // 记录跟踪日志
  ComponentHelp::GetTraceComponent()->TraceAccount(
      protoCheck.account(), pPacket->GetSocketKey()->Socket);
#endif

  // 向Redis查询在线状态
  Proto::AccountQueryOnlineToRedis protoToRedis;
  protoToRedis.set_account(pPlayer->GetAccount().c_str());
  MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_AccountQueryOnlineToRedis,
                                    protoToRedis, nullptr);

  // 将玩家请求的Http加入到代请求队列中
  if (!_playerSendHttpType.count(pPlayer->GetPlayerSN())) {
    _playerSendHttpType[pPlayer->GetPlayerSN()] = std::queue<Proto::MsgId>();
  }
  _playerSendHttpType[pPlayer->GetPlayerSN()].push(
      Proto::MsgId::C2L_AccountRegister);
}

// 处理Redis返回的查询在线状态的响应
void Account::HandleAccountQueryOnlineToRedisRs(Packet *pPacket) {
  // 从Packet中解析出Proto消息
  auto protoRs = pPacket->ParseToProto<Proto::AccountQueryOnlineToRedisRs>();

  // 获取玩家对象，通过账户信息查找
  auto pPlayer = GetComponent<PlayerCollectorComponent>()->GetPlayerByAccount(
      protoRs.account());
  if (pPlayer == nullptr)
    return; // 如果未找到玩家，直接返回

  // 如果玩家状态不是离线状态，则通知客户端账户在线，结束处理
  if (protoRs.return_code() !=
      Proto::AccountQueryOnlineToRedisRs::SOTR_Offline) {
    Proto::AccountCheckRs protoResult;
    protoResult.set_return_code(Proto::AccountCheckReturnCode::ARC_ONLINE);
    MessageSystemHelp::SendPacket(Proto::MsgId::C2L_AccountCheckRs, pPlayer,
                                  protoResult);
    return;
  }

  // 玩家离线状态，添加在线登录组件
  pPlayer->AddComponent<PlayerComponentOnlineInLogin>(pPlayer->GetAccount());

  TagValue tagValue{pPlayer->GetAccount(), 0};
  // 发起Http连接，验证账户信息
  MessageSystemHelp::CreateConnect(NetworkType::HttpConnector, TagType::Account,
                                   tagValue, _httpIp.c_str(), _httpPort);
}

void Account::HandleHttpOuterResponse(Packet *pPacket) {
  auto pTagValue = pPacket->GetTagKey()->GetTagValue(TagType::Account);
  if (pTagValue == nullptr) {
    LOG_ERROR("HandleHttpOuterResponse. can't find player.");
    return;
  }

  auto account = pTagValue->KeyStr;

  auto pPlayerCollector = GetComponent<PlayerCollectorComponent>();
  auto pPlayer = pPlayerCollector->GetPlayerByAccount(account);
  if (pPlayer == nullptr) {
    LOG_ERROR("http out response. but can't find player. account:"
              << account.c_str() << pPacket);
    return;
  }

  auto protoHttp = pPacket->ParseToProto<Proto::Http>();
  Proto::AccountCheckReturnCode rsCode =
      Proto::AccountCheckReturnCode::ARC_TIMEOUT;
  int statusCode = protoHttp.status_code();
  if (statusCode == 200) {
    auto response = protoHttp.body();
    const Json::CharReaderBuilder readerBuilder;
    Json::CharReader *jsonReader = readerBuilder.newCharReader();

    Json::Value value;
    JSONCPP_STRING errs;

    const bool ok = jsonReader->parse(
        response.data(), response.data() + response.size(), &value, &errs);
    if (ok && errs.empty()) {
      rsCode = ProcessMsg(value);
    } else {
      LOG_ERROR("json parse failed. " << response.c_str());
    }

    delete jsonReader;
  }

  // ���۳ɹ����ر�http����
  auto httpCallBackType = _httpCallbackType[pPacket->GetSocketKey()->Socket];
  _httpCallbackType.erase(pPacket->GetSocketKey()->Socket);
  MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_NetworkRequestDisconnect,
                                    pPacket);

  // ֪ͨ�ͻ��˽���lobby��ͼ
  auto pResMsg = ResourceHelp::GetResourceManager();
  auto pRolesMap = pResMsg->Worlds->GetRolesMap();
  if (pRolesMap != nullptr) {
    Proto::EnterWorld protoEnterWorld;
    protoEnterWorld.set_world_id(pRolesMap->GetId());
    MessageSystemHelp::SendPacket(Proto::MsgId::S2C_EnterWorld, pPlayer,
                                  protoEnterWorld);
  } else {
    LOG_ERROR("config error. not roles map.");
  }

  if (rsCode == Proto::AccountCheckReturnCode::ARC_OK) {

    // 登录请求成功后的操作
    if (httpCallBackType == Proto::MsgId::C2L_AccountCheck) {
      Proto::QueryPlayerList protoQuery;
      protoQuery.set_account(pPlayer->GetAccount().c_str());
      MessageSystemHelp::SendPacket(Proto::MsgId::L2DB_QueryPlayerList,
                                    protoQuery, APP_DB_MGR);
    }
  } else {

    // 登录失败请求结果返回给客户端
    if (httpCallBackType == Proto::MsgId::C2L_AccountCheck) {
      Proto::AccountCheckRs protoResult;
      protoResult.set_return_code(rsCode);
      MessageSystemHelp::SendPacket(Proto::MsgId::C2L_AccountCheckRs, pPlayer,
                                    protoResult);
    }
  }

  // 注册请求结果返回客户端
  if (httpCallBackType == Proto::MsgId::C2L_AccountRegister) {
    Proto::AccountCheckRs protoResult;
    protoResult.set_return_code(rsCode);
    MessageSystemHelp::SendPacket(Proto::MsgId::C2L_AccountRegisterRs, pPlayer,
                                  protoResult);
  }
}

// 处理HttpJson消息
Proto::AccountCheckReturnCode Account::ProcessMsg(Json::Value value) const {
  Proto::AccountCheckReturnCode code =
      Proto::AccountCheckReturnCode::ARC_UNKONWN; // 默认返回未知错误
  const int httpcode = value["code"].asInt();

  // 根据返回码设置对应的结果
  if (httpcode == 0)
    code = Proto::AccountCheckReturnCode::ARC_OK;
  else if (httpcode == 2)
    code = Proto::AccountCheckReturnCode::ARC_NOT_FOUND_ACCOUNT;
  else if (httpcode == 3)
    code = Proto::AccountCheckReturnCode::ARC_PASSWORD_WRONG;

  return code;
}

void Account::RequestToken(Player *pPlayer) const {
  // 创建一个用于请求 Redis 中 Token 的协议
  Proto::LoginTokenToRedis protoToken;
  protoToken.set_account(pPlayer->GetAccount().c_str()); // 设置账户名

  const auto pLoginInfo = pPlayer->GetComponent<PlayerComponentAccount>();
  protoToken.set_player_sn(pLoginInfo->GetSelectPlayerSn()); // 设置玩家 SN

  // 发送 Token 请求到 Redis
  MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_LoginTokenToRedis,
                                    protoToken, nullptr);
}

void Account::HandleTokenToRedisRs(Packet *pPacket) {
  // 解析 Token 返回的协议
  auto protoRs = pPacket->ParseToProto<Proto::LoginTokenToRedisRs>();
  const auto token = protoRs.token();

  // 根据账户获取玩家对象
  Player *pPlayer =
      GetComponent<PlayerCollectorComponent>()->GetPlayerByAccount(
          protoRs.account());
  if (pPlayer == nullptr) {
    LOG_WARN("can't find player. account:" << protoRs.account().c_str());
    return;
  }

  // 构建游戏 Token 协议
  Proto::GameToken protoToken;
  AppInfo info;
  if (!GetOneApp(APP_GAME, &info)) {
    protoToken.set_return_code(
        Proto::
            GameToken_ReturnCode_GameToken_NO_GAME); // 如果没找到游戏，设置返回码
  } else {
    protoToken.set_return_code(
        Proto::GameToken_ReturnCode_GameToken_OK); // 设置成功返回码
    protoToken.set_ip(info.Ip.c_str());            // 设置游戏服务器 IP
    protoToken.set_port(info.Port); // 设置游戏服务器端口
    protoToken.set_token(token);    // 设置 Token
  }
  // 发送游戏 Token 协议到客户端
  MessageSystemHelp::SendPacket(Proto::MsgId::L2C_GameToken, pPlayer,
                                protoToken);
}

void Account::HandleQueryPlayerListRs(Packet *pPacket) {
  auto protoRs = pPacket->ParseToProto<Proto::PlayerList>();
  std::string account = protoRs.account();

  auto pPlayerCollector = GetComponent<PlayerCollectorComponent>();

  const auto pPlayer = pPlayerCollector->GetPlayerByAccount(account);
  if (pPlayer == nullptr) {
    LOG_ERROR("HandleQueryPlayerLists. pPlayer == nullptr. account:"
              << account.c_str());
    return;
  }

  if (protoRs.player_size() > 0) {
    auto pListObj = pPlayer->GetComponent<PlayerComponentProtoList>();
    if (pListObj == nullptr)
      pListObj = pPlayer->AddComponent<PlayerComponentProtoList>();

    pListObj->Parse(protoRs);
  }

  // LOG_DEBUG("HandlePlayerListToDBRs account:" << account.c_str() << ", player
  // size:" << protoRs.player_size());

  // �����ת�͸��ͻ���
  MessageSystemHelp::SendPacket(Proto::MsgId::L2C_PlayerList, pPlayer, protoRs);
}

void Account::HandleCreatePlayer(Packet *pPacket) {
  auto pPlayerCollector = GetComponent<PlayerCollectorComponent>();

  auto protoCreate = pPacket->ParseToProto<Proto::CreatePlayer>();
  const auto pPlayer =
      pPlayerCollector->GetPlayerBySocket(pPacket->GetSocketKey()->Socket);
  if (pPlayer == nullptr) {
    LOG_ERROR("HandleCreatePlayer. pPlayer == nullptr. socket:"
              << pPacket->GetSocketKey()->Socket);
    return;
  }

  std::string account = pPlayer->GetAccount();
  LOG_DEBUG("create. account:" << account.c_str()
                               << " name:" << protoCreate.name().c_str()
                               << " gender:" << (int)protoCreate.gender());

  Proto::CreatePlayerToDB proto2Db;
  proto2Db.set_account(account.c_str());

  auto pProtoPlayer = proto2Db.mutable_player();
  pProtoPlayer->set_sn(Global::GetInstance()->GenerateSN());
  pProtoPlayer->set_name(protoCreate.name());
  pProtoPlayer->mutable_base()->set_gender(protoCreate.gender());

  MessageSystemHelp::SendPacket(Proto::MsgId::L2DB_CreatePlayer, proto2Db,
                                APP_DB_MGR);
}

void Account::HandleCreatePlayerRs(Packet *pPacket) {
  auto protoRs = pPacket->ParseToProto<Proto::CreatePlayerToDBRs>();
  std::string account = protoRs.account();

  auto pPlayerCollector = GetComponent<PlayerCollectorComponent>();

  auto pPlayer =
      pPlayerCollector->GetPlayerBySocket(pPacket->GetSocketKey()->Socket);
  if (pPlayer == nullptr) {
    LOG_ERROR("HandleCreatePlayerToDBRs. pPlayer == nullptr. account:"
              << account.c_str());
    return;
  }

  Proto::CreatePlayerRs createProto;
  createProto.set_return_code(protoRs.return_code());
  MessageSystemHelp::SendPacket(Proto::MsgId::C2L_CreatePlayerRs, pPlayer,
                                createProto);
}

void Account::HandleSelectPlayer(Packet *pPacket) {
  Proto::SelectPlayerRs protoRs;
  protoRs.set_return_code(Proto::SelectPlayerRs::SPRC_OK);

  auto proto = pPacket->ParseToProto<Proto::SelectPlayer>();
  auto pPlayerMgr = GetComponent<PlayerCollectorComponent>();
  auto pPlayer = pPlayerMgr->GetPlayerBySocket(pPacket->GetSocketKey()->Socket);
  if (pPlayer == nullptr) {
    LOG_ERROR("HandleSelectPlayer. pPlayer == nullptr");
    return;
  }

  uint64 selectPlayerSn = proto.player_sn();

  auto pPlayerLoginInfo = pPlayer->GetComponent<PlayerComponentAccount>();
  pPlayerLoginInfo->SetSelectPlayerSn(selectPlayerSn);

  do {
    if (pPlayer == nullptr) {
      protoRs.set_return_code(Proto::SelectPlayerRs::SPRC_NotFound);
      LOG_ERROR("HandleSelectPlayer. pPlayer == nullptr. " << pPacket);
      break;
    }

    auto pSubCompoent = pPlayer->GetComponent<PlayerComponentProtoList>();
    if (pSubCompoent == nullptr) {
      protoRs.set_return_code(Proto::SelectPlayerRs::SPRC_NotFound);
      LOG_ERROR("HandleSelectPlayer. pPlayer == nullptr. " << pPacket);
      break;
    }

    std::stringstream *pOpStream = pSubCompoent->GetProto(selectPlayerSn);
    if (pOpStream == nullptr) {
      protoRs.set_return_code(Proto::SelectPlayerRs::SPRC_NotFound);
      LOG_ERROR("HandleSelectPlayer. can't find player sn:" << selectPlayerSn);
      break;
    }

    pPlayer->ParseFromStream(selectPlayerSn, pOpStream);
  } while (false);

  if (Proto::SelectPlayerRs::SPRC_OK != protoRs.return_code()) {
    MessageSystemHelp::SendPacket(Proto::MsgId::C2L_SelectPlayerRs, pPlayer,
                                  protoRs);
    return;
  }

  //  ����token
  RequestToken(pPlayer);
}