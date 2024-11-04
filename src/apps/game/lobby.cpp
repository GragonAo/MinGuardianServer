#include "lobby.h"

#include "libserver/message_system_help.h"
#include "libplayer/player_collector_component.h"
#include "libplayer/player_component_proto_list.h"
#include "libserver/message_system.h"
#include "world_proxy_component_gather.h"
#include "player_component_onlinegame.h"
#include "player_component_token.h"
#include "libplayer/player.h"
#include "libplayer/player_component_last_map.h"
#include "world_proxy_help.h"
#include "world_proxy_locator.h"
#include "libresource/resource_help.h"
#include "libserver/socket_locator.h"

void Lobby::Awake()
{
    // 获取资源管理器并初始化世界 ID
    auto pResMgr = ResourceHelp::GetResourceManager();
    _worldId = pResMgr->Worlds->GetRolesMap()->GetId();

    // 添加组件
    AddComponent<PlayerCollectorComponent>();
    AddComponent<WorldProxyComponentGather>();

    // 注册世界代理定位器
    auto pProxyLocator = ComponentHelp::GetGlobalEntitySystem()->GetComponent<WorldProxyLocator>();
    pProxyLocator->RegisterToLocator(_worldId, GetSN());

    // 注册消息处理函数
    auto pMsgSystem = GetSystemManager()->GetMessageSystem();
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_NetworkDisconnect, BindFunP1(this, &Lobby::HandleNetworkDisconnect));
    pMsgSystem->RegisterFunction(this, Proto::MsgId::C2G_LoginByToken, BindFunP1(this, &Lobby::HandleLoginByToken));
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_GameTokenToRedisRs, BindFunP1(this, &Lobby::HandleGameTokenToRedisRs));
    pMsgSystem->RegisterFunction(this, Proto::MsgId::G2DB_QueryPlayerRs, BindFunP1(this, &Lobby::HandleQueryPlayerRs));
    pMsgSystem->RegisterFunction(this, Proto::MsgId::G2M_QueryWorldRs, BindFunP1(this, &Lobby::HandleQueryWorldRs));
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_BroadcastCreateWorldProxy, BindFunP1(this, &Lobby::HandleBroadcastCreateWorldProxy));
    pMsgSystem->RegisterFunctionFilter<Player>(this, Proto::MsgId::MI_TeleportAfter, BindFunP1(this, &Lobby::GetPlayer), BindFunP2(this, &Lobby::HandleTeleportAfter));
}

void Lobby::BackToPool()
{
    // 清空等待进入世界的玩家记录
    _waitingForWorld.clear();
}

Player* Lobby::GetPlayer(NetIdentify* pIdentify)
{
    // 获取玩家的 TagValue
    auto pTagValue = pIdentify->GetTagKey()->GetTagValue(TagType::Player);
    if (pTagValue == nullptr)
        return nullptr;

    const auto playerSn = pTagValue->KeyInt64;

    // 通过玩家收集器获取玩家对象
    auto pPlayerCollector = GetComponent<PlayerCollectorComponent>();
    return pPlayerCollector->GetPlayerBySn(playerSn);
}

void Lobby::HandleNetworkDisconnect(Packet* pPacket)
{
    // 处理网络断开事件
    auto pTagValue = pPacket->GetTagKey()->GetTagValue(TagType::Account);
    if (pTagValue == nullptr)
        return;

    // 从玩家收集器中移除玩家
    GetComponent<PlayerCollectorComponent>()->RemovePlayerBySocket(pPacket->GetSocketKey()->Socket);
}

void Lobby::HandleLoginByToken(Packet* pPacket)
{
    // 处理通过令牌登录的请求
    auto pPlayerCollector = GetComponent<PlayerCollectorComponent>();
    auto proto = pPacket->ParseToProto<Proto::LoginByToken>();
    auto pPlayer = pPlayerCollector->AddPlayer(pPacket, proto.account());
    
    if (pPlayer == nullptr)
    {
        MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_NetworkRequestDisconnect, pPacket);
        return;
    }

    // 添加令牌和在线游戏组件
    pPlayer->AddComponent<PlayerComponentToken>(proto.token());
    pPlayer->AddComponent<PlayerComponentOnlineInGame>(pPlayer->GetAccount(), 1);

    // 发送游戏令牌到 Redis
    Proto::GameTokenToRedis protoToken;
    protoToken.set_account(pPlayer->GetAccount().c_str());
    MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_GameTokenToRedis, protoToken, nullptr);

    // 注册 Socket
    auto pSocketLocator = ComponentHelp::GetGlobalEntitySystem()->GetComponent<SocketLocator>();
    pSocketLocator->RegisterToLocator(pPlayer->GetSocketKey()->Socket, GetSN());
}

void Lobby::HandleGameTokenToRedisRs(Packet* pPacket)
{
    // 处理游戏令牌从 Redis 返回的结果
    auto protoRs = pPacket->ParseToProto<Proto::GameTokenToRedisRs>();
    auto pPlayer = GetComponent<PlayerCollectorComponent>()->GetPlayerByAccount(protoRs.account());
    
    if (pPlayer == nullptr)
    {
        LOG_ERROR("HandleGameRequestTokenToRedisRs. pPlayer == nullptr. account:" << protoRs.account().c_str());
        return;
    }

    Proto::LoginByTokenRs protoLoginGameRs;
    protoLoginGameRs.set_return_code(Proto::LoginByTokenRs::LGRC_TOKEN_WRONG);
    const auto pTokenComponent = pPlayer->GetComponent<PlayerComponentToken>();

    // 验证令牌是否有效
    if (pTokenComponent->IsTokenValid(protoRs.token_info().token()))
    {
        protoLoginGameRs.set_return_code(Proto::LoginByTokenRs::LGRC_OK);
    }

    MessageSystemHelp::SendPacket(Proto::MsgId::C2G_LoginByTokenRs, pPlayer,protoLoginGameRs);

    if (protoLoginGameRs.return_code() != Proto::LoginByTokenRs::LGRC_OK)
        return;

    // 查询玩家信息
    Proto::QueryPlayer protoQuery;
    protoQuery.set_player_sn(protoRs.token_info().player_sn());
    MessageSystemHelp::SendPacket(Proto::MsgId::G2DB_QueryPlayer, protoQuery, APP_DB_MGR);
}

void Lobby::HandleQueryPlayerRs(Packet* pPacket)
{
    // 处理查询玩家返回结果
    auto protoRs = pPacket->ParseToProto<Proto::QueryPlayerRs>();
    auto account = protoRs.account();
    auto pPlayer = GetComponent<PlayerCollectorComponent>()->GetPlayerByAccount(account);
    
    if (pPlayer == nullptr)
    {
        LOG_ERROR("HandleQueryPlayer. pPlayer == nullptr. account:" << account.c_str());
        return;
    }

    // 同步玩家信息
    Proto::SyncPlayer syncPlayer;
    syncPlayer.mutable_player()->CopyFrom(protoRs.player());
    MessageSystemHelp::SendPacket(Proto::MsgId::G2C_SyncPlayer, pPlayer,syncPlayer);

    // 解析玩家信息
    auto protoPlayer = protoRs.player();
    const auto playerSn = protoPlayer.sn();
    pPlayer->ParserFromProto(playerSn, protoPlayer);
    const auto pPlayerLastMap = pPlayer->AddComponent<PlayerComponentLastMap>();
    auto pWorldLocator = ComponentHelp::GetGlobalEntitySystem()->GetComponent<WorldProxyLocator>();

    // 处理最后副本
    auto pLastMap = pPlayerLastMap->GetLastDungeon();
    if (pLastMap != nullptr)
    {
        if (pWorldLocator->IsExistDungeon(pLastMap->WorldSn))
        {
            // 传送到最后副本
            WorldProxyHelp::Teleport(pPlayer, GetSN(), pLastMap->WorldSn);
            return;
        }

        // 检查是否在等待列表中
        if (_waitingForDungeon.find(pLastMap->WorldSn) == _waitingForDungeon.end())
        {
            _waitingForDungeon[pLastMap->WorldSn] = std::set<uint64>();
        }

        if (_waitingForDungeon[pLastMap->WorldSn].empty())
        {
            // 向 AppMgr 查询副本信息
            Proto::QueryWorld protoToMgr;
            protoToMgr.set_world_sn(pLastMap->WorldSn);
            protoToMgr.set_last_world_sn(GetSN());
            MessageSystemHelp::SendPacket(Proto::MsgId::G2M_QueryWorld, protoToMgr, APP_APPMGR);
        }

        _waitingForDungeon[pLastMap->WorldSn].insert(pPlayer->GetPlayerSN());
        return;
    }

    // 进入公共世界
    EnterPublicWorld(pPlayer);
}

// 进入公共世界
void Lobby::EnterPublicWorld(Player* pPlayer)
{
    const auto pPlayerLastMap = pPlayer->GetComponent<PlayerComponentLastMap>();
    auto pWorldLocator = ComponentHelp::GetGlobalEntitySystem()->GetComponent<WorldProxyLocator>();

    // 获取最后公共地图
    auto pLastMap = pPlayerLastMap->GetLastPublicMap();
    const auto lastMapSn = pWorldLocator->GetWorldSnById(pLastMap->WorldId);
    
    if (lastMapSn != (uint64)INVALID_ID)
    {
        // 传送到公共世界
        WorldProxyHelp::Teleport(pPlayer, GetSN(), lastMapSn);
        return;
    }

    // 等待传送
    if (_waitingForWorld.find(pLastMap->WorldId) == _waitingForWorld.end())
    {
        _waitingForWorld[pLastMap->WorldId] = std::set<uint64>();
    }

    if (_waitingForWorld[pLastMap->WorldId].empty())
    {
        // 向 appmgr 请求创建世界
        Proto::RequestWorld protoToMgr;
        protoToMgr.set_world_id(pLastMap->WorldId);
        MessageSystemHelp::SendPacket(Proto::MsgId::G2M_RequestWorld, protoToMgr, APP_APPMGR);
    }

    _waitingForWorld[pLastMap->WorldId].insert(pPlayer->GetPlayerSN());
}

void Lobby::HandleQueryWorldRs(Packet* pPacket)
{
    // 处理查询世界返回结果
    auto proto = pPacket->ParseToProto<Proto::QueryWorldRs>();
    const auto worldSn = proto.world_sn();

    const auto iter = _waitingForDungeon.find(worldSn);
    if (iter == _waitingForDungeon.end())
        return;

    if (proto.return_code() == Proto::QueryWorldRs::QueryWorld_OK)
        return;

    auto pPlayerMgr = GetComponent<PlayerCollectorComponent>();

    // 遍历等待的玩家并将他们传送到公共世界
    auto players = iter->second;
    for (auto one : players)
    {
        const auto pPlayer = pPlayerMgr->GetPlayerBySn(one);
        if (pPlayer == nullptr)
            continue;

        EnterPublicWorld(pPlayer);
    }

    _waitingForDungeon.erase(iter);
}

void Lobby::HandleBroadcastCreateWorldProxy(Packet* pPacket)
{
    // 处理创建世界代理的广播消息
    auto proto = pPacket->ParseToProto<Proto::BroadcastCreateWorldProxy>();
    const auto worldId = proto.world_id();
    const auto worldSn = proto.world_sn();

    auto pResMgr = ResourceHelp::GetResourceManager();
    auto pWorldRes = pResMgr->Worlds->GetResource(worldId);
    if (pWorldRes == nullptr)
    {
        LOG_ERROR("can't find resources of world. world id:" << worldId);
        return;
    }

    if (pWorldRes->IsType(ResourceWorldType::Public))
    {
        const auto iter = _waitingForWorld.find(worldId);
        if (iter == _waitingForWorld.end())
            return;

        auto pPlayerMgr = GetComponent<PlayerCollectorComponent>();

        // 将等待的玩家传送到新创建的公共世界
        auto players = iter->second;
        for (auto one : players)
        {
            const auto player = pPlayerMgr->GetPlayerBySn(one);
            if (player == nullptr)
                continue;

            WorldProxyHelp::Teleport(player, GetSN(), worldSn);
        }

        _waitingForWorld.erase(iter);
    }
    else
    {
        // 处理私有世界的情况
        auto iter = _waitingForDungeon.find(worldSn);
        if (iter == _waitingForDungeon.end())
        {
            LOG_ERROR("can't find player. world id:" << worldId);
            return;
        }
        
        auto pPlayerMgr = GetComponent<PlayerCollectorComponent>();
        auto players = iter->second;
        for (auto one : players)
        {
            const auto player = pPlayerMgr->GetPlayerBySn(one);
            if (player == nullptr)
                continue;

            WorldProxyHelp::Teleport(player, GetSN(), worldSn);
        }

        _waitingForDungeon.erase(iter);
    }
}

void Lobby::HandleTeleportAfter(Player* pPlayer, Packet* pPacket)
{
    // 处理传送后移除玩家的操作
    auto pPlayerMgr = GetComponent<PlayerCollectorComponent>();
    pPlayerMgr->RemovePlayerBySocket(pPlayer->GetSocketKey()->Socket);
}

