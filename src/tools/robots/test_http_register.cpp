#include "test_http_register.h"
#include "libserver/component_help.h"
#include "libserver/message_system.h"
#include "libserver/message_system_help.h"

void TestHttpRegister::Awake(std::string account, std::string password) {
  _account = account;
  _password = password;

  _tagKey.AddTag(TagType::Account, _account);

  // message
  auto pMsgSystem = GetSystemManager()->GetMessageSystem();

  // ��������
  pMsgSystem->RegisterFunctionFilter<TestHttpRegister>(
      this, Proto::MsgId::MI_NetworkDisconnect,
      BindFunP1(this, &TestHttpRegister::GetTestHttpRegister),
      BindFunP2(this, &TestHttpRegister::HandleNetworkDisconnect));
  pMsgSystem->RegisterFunctionFilter<TestHttpRegister>(
      this, Proto::MsgId::MI_NetworkConnected,
      BindFunP1(this, &TestHttpRegister::GetTestHttpRegister),
      BindFunP2(this, &TestHttpRegister::HandleNetworkConnected));

  pMsgSystem->RegisterFunctionFilter<TestHttpRegister>(
      this, Proto::MsgId::MI_HttpOuterResponse,
      BindFunP1(this, &TestHttpRegister::GetTestHttpRegister),
      BindFunP2(this, &TestHttpRegister::HandleHttpOuterResponse));

  // ��������
  auto pYaml = ComponentHelp::GetYaml();
  const auto pLoginConfig =
      dynamic_cast<LoginConfig *>(pYaml->GetConfig(APP_LOGIN));
  ParseUrlInfo info;
  if (!MessageSystemHelp::ParseUrl(pLoginConfig->UrlRegister, info)) {
    LOG_ERROR("parse login url failed. url:" << pLoginConfig->UrlRegister.c_str());
    return;
  }

  _ip = info.Host;
  _port = info.Port;
  _mothed = pLoginConfig->UrlMethod;
  _path = info.Path;

  TagValue tagValue{_account, 0L};
  MessageSystemHelp::CreateConnect(NetworkType::HttpConnector, TagType::Account,
                                   tagValue, _ip, _port);
}

void TestHttpRegister::BackToPool() {}

TestHttpRegister *TestHttpRegister::GetTestHttpRegister(NetIdentify *pNetIdentify) {
  auto pTagValue = pNetIdentify->GetTagKey()->GetTagValue(TagType::Account);
  if (pTagValue == nullptr)
    return nullptr;

  if (pTagValue->KeyStr == _account)
    return this;

  return nullptr;
}

void TestHttpRegister::HandleNetworkDisconnect(TestHttpRegister *pObj,
                                            Packet *pPacket) {
  LOG_ERROR("test http login. recv network disconnect.");
}

void TestHttpRegister::HandleNetworkConnected(TestHttpRegister *pObj,
                                           Packet *pPacket) {
  // ���ӳɹ�����������
  _socketKey.CopyFrom(pPacket->GetSocketKey());
  LOG_DEBUG("connected." << pPacket);

  std::map<std::string, std::string> params;
  params["account"] = _account;
  params["password"] = _password;
  MessageSystemHelp::SendHttpRequest(this, _ip, _port, _mothed, _path, &params);
}

void TestHttpRegister::HandleHttpOuterResponse(TestHttpRegister *pObj,
                                            Packet *pPacket) {
  auto protoHttp = pPacket->ParseToProto<Proto::Http>();
  LOG_DEBUG("http code:" << protoHttp.status_code());
  LOG_DEBUG(protoHttp.body().c_str());
}
