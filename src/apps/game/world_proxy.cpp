#include "world_proxy.h"
#include "world_proxy_locator.h"
#include "player_component_onlinegame.h"

#include "libserver/component_help.h"
#include "libserver/message_system_help.h"
#include "libserver/message_system.h"

#include "libplayer/player_collector_component.h"
#include "libplayer/player.h"

#include "libserver/log4.h"
#include "libserver/network_help.h"

#include "world_proxy_component_gather.h"
#include "world_component_teleport.h"
#include "libresource/resource_help.h"
#include "libserver/socket_locator.h"

void WorldProxy::Awake(int worldId, uint64 lastWorldSn)
{
    _worldId = worldId;
    _spaceAppId = Global::GetAppIdFromSN(_sn);

    // 添加所需的组件
    AddComponent<PlayerCollectorComponent>();
    AddComponent<WorldProxyComponentGather>();
    AddComponent<WorldComponentTeleport>();

    // 注册到定位器
    auto pProxyLocator = ComponentHelp::GetGlobalEntitySystem()->GetComponent<WorldProxyLocator>();
    pProxyLocator->RegisterToLocator(_worldId, GetSN());

    // 创建世界代理的广播消息
    Proto::BroadcastCreateWorldProxy protoCreate;
    protoCreate.set_world_id(_worldId);
    protoCreate.set_world_sn(GetSN());

    // 根据是否有前一个世界序号进行广播
    if (lastWorldSn > 0)
    {
        NetIdentify netIdentify;
        netIdentify.GetTagKey()->AddTag(TagType::Entity, lastWorldSn);
        MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_BroadcastCreateWorldProxy, protoCreate, &netIdentify);
    }
    else
    {
        MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_BroadcastCreateWorldProxy, protoCreate, nullptr);
    }

    // 注册消息处理函数
    auto pMsgSystem = GetSystemManager()->GetMessageSystem();
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_NetworkDisconnect, BindFunP1(this, &WorldProxy::HandleNetworkDisconnect));
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_Teleport, BindFunP1(this, &WorldProxy::HandleTeleport));
    pMsgSystem->RegisterFunctionFilter<Player>(this, Proto::MsgId::MI_TeleportAfter, BindFunP1(this, &WorldProxy::GetPlayer), BindFunP2(this, &WorldProxy::HandleTeleportAfter));
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_BroadcastCreateWorldProxy, BindFunP1(this, &WorldProxy::HandleBroadcastCreateWorldProxy));
    pMsgSystem->RegisterFunctionFilter<Player>(this, Proto::MsgId::S2G_SyncPlayer, BindFunP1(this, &WorldProxy::GetPlayer), BindFunP2(this, &WorldProxy::HandleS2GSyncPlayer));
    pMsgSystem->RegisterFunctionFilter<Player>(this, Proto::MsgId::C2G_EnterWorld, BindFunP1(this, &WorldProxy::GetPlayer), BindFunP2(this, &WorldProxy::HandleC2GEnterWorld));
    pMsgSystem->RegisterDefaultFunction(this, BindFunP1(this, &WorldProxy::HandleDefaultFunction));
}

void WorldProxy::BackToPool()
{
    // 这里可以实现回收资源的逻辑
}

void WorldProxy::SendPacketToWorld(const Proto::MsgId msgId, ::google::protobuf::Message& proto, Player* pPlayer) const
{
    TagKey tagKey;
    tagKey.AddTag(TagType::Player, pPlayer->GetPlayerSN());
    tagKey.AddTag(TagType::Entity, _sn);
    if (Global::GetInstance()->GetCurAppType() == APP_ALLINONE)
    {
        tagKey.AddTag(TagType::ToWorld, _sn);
    }
    MessageSystemHelp::SendPacket(msgId, proto, &tagKey, APP_SPACE, _spaceAppId);
}

void WorldProxy::SendPacketToWorld(const Proto::MsgId msgId, Player* pPlayer) const
{
    TagKey tagKey;
    tagKey.AddTag(TagType::Player, pPlayer->GetPlayerSN());
    tagKey.AddTag(TagType::Entity, _sn);
    if (Global::GetInstance()->GetCurAppType() == APP_ALLINONE)
    {
        tagKey.AddTag(TagType::ToWorld, _sn);
    }
    MessageSystemHelp::SendPacket(msgId, &tagKey, APP_SPACE, _spaceAppId);
}

void WorldProxy::CopyPacketToWorld(Player* pPlayer, Packet* pPacket) const
{
    auto pPacketCopy = MessageSystemHelp::CreatePacket((Proto::MsgId)pPacket->GetMsgId(), nullptr);
    pPacketCopy->CopyFrom(pPacket);
    auto pTagKey = pPacketCopy->GetTagKey();
    pTagKey->AddTag(TagType::Player, pPlayer->GetPlayerSN());
    pTagKey->AddTag(TagType::Entity, _sn);
    if (Global::GetInstance()->GetCurAppType() == APP_ALLINONE)
    {
        pTagKey->AddTag(TagType::ToWorld, _sn);
    }

    MessageSystemHelp::SendPacket(pPacketCopy, APP_SPACE, _spaceAppId);
}

Player* WorldProxy::GetPlayer(NetIdentify* pIdentify)
{
    auto pTags = pIdentify->GetTagKey();
    const auto pTagAccount = pTags->GetTagValue(TagType::Account);
    if (pTagAccount != nullptr)
    {
        auto pPlayerMgr = this->GetComponent<PlayerCollectorComponent>();
        return pPlayerMgr->GetPlayerBySocket(pIdentify->GetSocketKey()->Socket);
    }

    const auto pTagPlayer = pTags->GetTagValue(TagType::Player);
    if (pTagPlayer != nullptr)
    {
        auto pPlayerMgr = this->GetComponent<PlayerCollectorComponent>();
        return pPlayerMgr->GetPlayerBySn(pTagPlayer->KeyInt64);
    }

    return nullptr;
}

void WorldProxy::HandleDefaultFunction(Packet* pPacket)
{
    auto pPlayerMgr = this->GetComponent<PlayerCollectorComponent>();
    Player* pPlayer = nullptr;
    const auto pTagKey = pPacket->GetTagKey();
    if (pTagKey == nullptr)
    {
        LOG_ERROR("world proxy recv msg. but no tag. msgId:" << Log4Help::GetMsgIdName(pPacket->GetMsgId()).c_str());
        return;
    }

    bool isToClient = false;
    const auto pTagPlayer = pTagKey->GetTagValue(TagType::Player);
    if (pTagPlayer != nullptr)
    {
        isToClient = true;
        pPlayer = pPlayerMgr->GetPlayerBySn(pTagPlayer->KeyInt64);
    }
    else
    {
        pPlayer = pPlayerMgr->GetPlayerBySocket(pPacket->GetSocketKey()->Socket);
    }

    // 玩家为空则返回
    if (pPlayer == nullptr)
        return;

    // 根据消息是否是发给客户端进行不同处理
    if (isToClient)
    {
        auto pPacketCopy = MessageSystemHelp::CreatePacket((Proto::MsgId)pPacket->GetMsgId(), pPlayer);
        pPacketCopy->CopyFrom(pPacket);
        MessageSystemHelp::SendPacket(pPacketCopy);
    }
    else
    {
        CopyPacketToWorld(pPlayer, pPacket);
    }
}

void WorldProxy::HandleNetworkDisconnect(Packet* pPacket)
{
    if (!NetworkHelp::IsTcp(pPacket->GetSocketKey()->NetType))
        return;

    TagValue* pTagValue = pPacket->GetTagKey()->GetTagValue(TagType::Account);
    if (pTagValue != nullptr)
    {
        const auto pPlayerCollector = GetComponent<PlayerCollectorComponent>();
        if (pPlayerCollector == nullptr)
            return;

        const auto pPlayer = pPlayerCollector->GetPlayerBySocket(pPacket->GetSocketKey()->Socket);
        if (pPlayer == nullptr)
            return;

        auto pCollector = GetComponent<PlayerCollectorComponent>();
        pCollector->RemovePlayerBySocket(pPacket->GetSocketKey()->Socket);

        SendPacketToWorld(Proto::MsgId::MI_NetworkDisconnect, pPlayer);
    }
    else
    {
        // 处理非玩家断开连接的情况
        auto pTags = pPacket->GetTagKey();
        const auto pTagApp = pTags->GetTagValue(TagType::App);
        if (pTagApp == nullptr)
            return;

        const auto appKey = pTagApp->KeyInt64;
        const auto appType = GetTypeFromAppKey(appKey);
        const auto appId = GetIdFromAppKey(appKey);

        if (appType != APP_SPACE || _spaceAppId != appId)
            return;

        // 移除所有玩家并关闭连接
        auto pPlayerCollector = GetComponent<PlayerCollectorComponent>();
        pPlayerCollector->RemoveAllPlayerAndCloseConnect();

        // 从定位器中移除
        auto pWorldLocator = ComponentHelp::GetGlobalEntitySystem()->GetComponent<WorldProxyLocator>();
        pWorldLocator->Remove(_worldId, GetSN());

        // 删除当前代理组件
        GetSystemManager()->GetEntitySystem()->RemoveComponent(this);
    }
}

void WorldProxy::HandleTeleport(Packet* pPacket)
{
    // 解析传送请求的消息
    auto proto = pPacket->ParseToProto<Proto::Teleport>();
    const auto playerSn = proto.player_sn();

    // 从玩家收集器中添加玩家
    auto pCollector = GetComponent<PlayerCollectorComponent>();
    auto pPlayer = pCollector->AddPlayer(pPacket, proto.account());
    if (pPlayer == nullptr)
    {
        LOG_ERROR("failed to teleport, account:" << proto.account().c_str());
        return;
    }

    // 从消息中解析玩家信息并添加在线游戏组件
    pPlayer->ParserFromProto(playerSn, proto.player());
    pPlayer->AddComponent<PlayerComponentOnlineInGame>(pPlayer->GetAccount());

    // 创建同步玩家消息并发送到世界
    Proto::SyncPlayer protoSync;
    protoSync.set_account(proto.account().c_str());
    protoSync.mutable_player()->CopyFrom(proto.player());
    SendPacketToWorld(Proto::MsgId::G2S_SyncPlayer, protoSync, pPlayer);

    // 通知客户端传送成功
    Proto::TeleportAfter protoTeleportRs;
    protoTeleportRs.set_player_sn(pPlayer->GetPlayerSN());
    NetIdentify identify;
    identify.GetTagKey()->AddTag(TagType::Player, pPlayer->GetPlayerSN());
    identify.GetTagKey()->AddTag(TagType::Entity, proto.last_world_sn());
    MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_TeleportAfter, protoTeleportRs, &identify);

    // 注册玩家的 socket
    auto pSocketLocator = ComponentHelp::GetGlobalEntitySystem()->GetComponent<SocketLocator>();
    pSocketLocator->RegisterToLocator(pPlayer->GetSocketKey()->Socket, GetSN());
}

void WorldProxy::HandleTeleportAfter(Player* pPlayer, Packet* pPacket)
{
    // 处理传送后的玩家状态
    auto proto = pPacket->ParseToProto<Proto::TeleportAfter>();
    const auto playerSn = proto.player_sn();

    // 从玩家收集器中移除玩家
    auto pPlayerMgr = GetComponent<PlayerCollectorComponent>();
    pPlayerMgr->RemovePlayerBySocket(pPlayer->GetSocketKey()->Socket);

    // 创建移除玩家消息并发送到世界
    Proto::RemovePlayer protoRs;
    protoRs.set_player_sn(playerSn);
    SendPacketToWorld(Proto::MsgId::G2S_RemovePlayer, protoRs, pPlayer);
}

void WorldProxy::HandleC2GEnterWorld(Player* pPlayer, Packet* pPacket)
{
    // 处理玩家进入世界的请求
    auto proto = pPacket->ParseToProto<Proto::EnterWorld>();
    auto worldId = proto.world_id();
    const auto pResMgr = ResourceHelp::GetResourceManager();
    const auto pWorldRes = pResMgr->Worlds->GetResource(worldId);

    // 检查世界资源是否有效
    if (pWorldRes == nullptr)
        return;

    // 检查玩家是否正在传送
    auto pTeleportComponent = this->GetComponent<WorldComponentTeleport>();
    if (pTeleportComponent->IsTeleporting(pPlayer))
        return;

    // 创建传送对象以进行世界传送
    GetComponent<WorldComponentTeleport>()->CreateTeleportObject(worldId, pPlayer);
}

void WorldProxy::HandleS2GSyncPlayer(Player* pPlayer, Packet* pPacket)
{
    // 处理同步玩家状态的请求
    auto proto = pPacket->ParseToProto<Proto::SyncPlayer>();
    pPlayer->ParserFromProto(pPlayer->GetPlayerSN(), proto.player());
    GetComponent<WorldComponentTeleport>()->BroadcastSyncPlayer(pPlayer->GetPlayerSN());
}

void WorldProxy::HandleBroadcastCreateWorldProxy(Packet* pPacket)
{
    // 处理广播创建世界代理的消息
    auto proto = pPacket->ParseToProto<Proto::BroadcastCreateWorldProxy>();
    GetComponent<WorldComponentTeleport>()->HandleBroadcastCreateWorldProxy(proto.world_id(), proto.world_sn());
}
