#include "world.h"
#include "player_manager_component.h"
#include "player_component_detail.h"

#include "libserver/message_system_help.h"
#include "libserver/message_system.h"
#include "libplayer/player_component_last_map.h"
#include "move_component.h"

// Awake 方法用于初始化世界，设置世界 ID，添加组件和定时器。
void World::Awake(int worldId)
{
    // LOG_DEBUG("create world. id:" << worldId << " sn:" << _sn << " space app id:" << Global::GetAppIdFromSN(_sn));

    _worldId = worldId;

    AddComponent<PlayerManagerComponent>();  // 添加玩家管理组件

    // 设置定时器，定期同步信息
    AddTimer(0, 10, false, 0, BindFunP0(this, &World::SyncWorldToGather));
    AddTimer(0, 1, false, 0, BindFunP0(this, &World::SyncAppearTimer));

    // 注册网络消息处理函数
    auto pMsgSystem = GetSystemManager()->GetMessageSystem();
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_NetworkDisconnect, BindFunP1(this, &World::HandleNetworkDisconnect));
    pMsgSystem->RegisterFunction(this, Proto::MsgId::G2S_SyncPlayer, BindFunP1(this, &World::HandleSyncPlayer));

    // 注册与玩家相关的消息过滤函数
    pMsgSystem->RegisterFunctionFilter<Player>(this, Proto::MsgId::G2S_RequestSyncPlayer, BindFunP1(this, &World::GetPlayer), BindFunP2(this, &World::HandleRequestSyncPlayer));
    pMsgSystem->RegisterFunctionFilter<Player>(this, Proto::MsgId::G2S_RemovePlayer, BindFunP1(this, &World::GetPlayer), BindFunP2(this, &World::HandleG2SRemovePlayer));
    pMsgSystem->RegisterFunctionFilter<Player>(this, Proto::MsgId::C2S_Move, BindFunP1(this, &World::GetPlayer), BindFunP2(this, &World::HandleMove));
}

// BackToPool 方法在组件被放回对象池时调用，用于清理状态。
void World::BackToPool()
{
    _addPlayer.clear();
}

// 根据网络标识获取玩家
Player* World::GetPlayer(NetIdentify* pIdentify)
{
    auto pTags = pIdentify->GetTagKey();
    const auto pTagPlayer = pTags->GetTagValue(TagType::Player);
    if (pTagPlayer == nullptr)
        return nullptr;

    auto pPlayerMgr = GetComponent<PlayerManagerComponent>();
    return pPlayerMgr->GetPlayerBySn(pTagPlayer->KeyInt64);
}

// 处理网络断开事件
void World::HandleNetworkDisconnect(Packet* pPacket)
{
    // LOG_DEBUG("world id:" << _worldId << " disconnect." << pPacket);

    auto pTags = pPacket->GetTagKey();
    const auto pTagPlayer = pTags->GetTagValue(TagType::Player);
    if (pTagPlayer != nullptr)
    {
        auto pPlayerMgr = GetComponent<PlayerManagerComponent>();
        const auto pPlayer = pPlayerMgr->GetPlayerBySn(pTagPlayer->KeyInt64);
        if (pPlayer == nullptr)
        {
            LOG_ERROR("world. net disconnect. can't find player. player sn:" << pTagPlayer->KeyInt64);
            return;
        }

        // 保存玩家数据到数据库
        Proto::SavePlayer protoSave;
        protoSave.set_player_sn(pPlayer->GetPlayerSN());
        pPlayer->SerializeToProto(protoSave.mutable_player());
        MessageSystemHelp::SendPacket(Proto::MsgId::G2DB_SavePlayer, protoSave, APP_DB_MGR);

        // 从玩家管理器中移除玩家
        pPlayerMgr->RemovePlayerBySn(pTagPlayer->KeyInt64);
    }
    else
    {
        // 处理应用程序管理或数据库管理的断开情况
        const auto pTagApp = pTags->GetTagValue(TagType::App);
        if (pTagApp != nullptr)
        {
            auto pPlayerMgr = GetComponent<PlayerManagerComponent>();
            pPlayerMgr->RemoveAllPlayers(pPacket);
        }
    }
}

// 同步世界信息到收集器
void World::SyncWorldToGather()
{
    Proto::WorldSyncToGather proto;
    proto.set_world_sn(GetSN());
    proto.set_world_id(GetWorldId());

    const int online = GetComponent<PlayerManagerComponent>()->OnlineSize();
    proto.set_online(online);

    MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_WorldSyncToGather, proto, nullptr);
}

// 创建角色出现的协议消息
inline void CreateProtoRoleAppear(Player* pPlayer, Proto::RoleAppear& protoAppear)
{
    auto proto = protoAppear.add_role();
    proto->set_name(pPlayer->GetName().c_str());
    proto->set_sn(pPlayer->GetPlayerSN());

    const auto pBaseInfo = pPlayer->GetComponent<PlayerComponentDetail>();
    proto->set_gender(pBaseInfo->GetGender());

    const auto pComponentLastMap = pPlayer->GetComponent<PlayerComponentLastMap>();
    const auto pLastMap = pComponentLastMap->GetCur();
    pLastMap->Position.SerializeToProto(proto->mutable_position());
}

// 同步玩家出现信息
void World::SyncAppearTimer()
{
    auto pPlayerMgr = GetComponent<PlayerManagerComponent>();

    if (!_addPlayer.empty())
    {
        // 1. 收集新加入的玩家信息
        Proto::RoleAppear protoNewAppear;
        for (auto id : _addPlayer)
        {
            const auto pPlayer = pPlayerMgr->GetPlayerBySn(id);
            if (pPlayer == nullptr)
                continue;

            CreateProtoRoleAppear(pPlayer, protoNewAppear);
        }

        // 广播新玩家的出现
        if (protoNewAppear.role_size() > 0)
            BroadcastPacket(Proto::MsgId::S2C_RoleAppear, protoNewAppear);

        // 2. 收集已经存在的玩家信息
        Proto::RoleAppear protoOther;
        const auto players = pPlayerMgr->GetAll();
        for (const auto one : *players)
        {
            if (_addPlayer.find(one.first) != _addPlayer.end())
                continue;

            const auto role = one.second;
            CreateProtoRoleAppear(role, protoOther);
        }

        // 广播已有玩家的出现
        if (protoOther.role_size() > 0)
            BroadcastPacket(Proto::MsgId::S2C_RoleAppear, protoOther, _addPlayer);

        _addPlayer.clear();
    }
}

// 处理同步玩家的消息
void World::HandleSyncPlayer(Packet* pPacket)
{
    auto proto = pPacket->ParseToProto<Proto::SyncPlayer>();
    const auto playerSn = proto.player().sn();

    auto pPlayerMgr = GetComponent<PlayerManagerComponent>();
    auto pPlayer = pPlayerMgr->AddPlayer(playerSn, GetSN(), pPacket);
    if (pPlayer == nullptr)
    {
        LOG_ERROR("failed to add player. player sn:" << playerSn);
        return;
    }

    pPlayer->ParserFromProto(playerSn, proto.player());
    pPlayer->AddComponent<PlayerComponentDetail>();

    const auto pComponentLastMap = pPlayer->AddComponent<PlayerComponentLastMap>();
    pComponentLastMap->EnterWorld(_worldId, _sn);
    const auto pLastMap = pComponentLastMap->GetCur();

    // 通知客户端进入世界
    Proto::EnterWorld protoEnterWorld;
    protoEnterWorld.set_world_id(_worldId);
    pLastMap->Position.SerializeToProto(protoEnterWorld.mutable_position());
    MessageSystemHelp::SendPacket(Proto::MsgId::S2C_EnterWorld, pPlayer, protoEnterWorld);

    _addPlayer.insert(playerSn);  // 将新玩家添加到集合中
}

// 广播消息给所有玩家
void World::BroadcastPacket(Proto::MsgId msgId, google::protobuf::Message& proto)
{
    auto pPlayerMgr = GetComponent<PlayerManagerComponent>();
    const auto players = pPlayerMgr->GetAll();
    for (const auto pair : *players)
    {
        MessageSystemHelp::SendPacket(msgId, pair.second, proto);
    }
}

// 广播消息给指定玩家
void World::BroadcastPacket(Proto::MsgId msgId, google::protobuf::Message& proto, std::set<uint64> players)
{
    auto pPlayerMgr = GetComponent<PlayerManagerComponent>();
    for (const auto one : players)
    {
        const auto pPlayer = pPlayerMgr->GetPlayerBySn(one);
        if (pPlayer == nullptr)
            continue;

        MessageSystemHelp::SendPacket(msgId, pPlayer, proto);
    }
}

// 处理请求同步玩家信息的消息
void World::HandleRequestSyncPlayer(Player* pPlayer, Packet* pPacket)
{
    Proto::SyncPlayer protoSync;
    protoSync.set_account(pPlayer->GetAccount().c_str());
    pPlayer->SerializeToProto(protoSync.mutable_player());

    MessageSystemHelp::SendPacket(Proto::MsgId::S2G_SyncPlayer, pPlayer, protoSync);
}

// 处理移除玩家的消息
void World::HandleG2SRemovePlayer(Player* pPlayer, Packet* pPacket)
{
    // 解析从客户端收到的移除玩家的消息
    auto proto = pPacket->ParseToProto<Proto::RemovePlayer>();
    
    // 检查消息中的玩家序列号是否与传入的玩家一致
    if (proto.player_sn() != pPlayer->GetPlayerSN())
    {
        // 日志错误信息，说明玩家序列号不匹配
        LOG_ERROR("HandleTeleportAfter. proto.player_sn() != pPlayer->GetPlayerSN()");
        return; // 如果不匹配，直接返回
    }

    // 获取玩家管理组件
    auto pPlayerMgr = GetComponent<PlayerManagerComponent>();
    // 从管理器中移除该玩家
    pPlayerMgr->RemovePlayerBySn(pPlayer->GetPlayerSN());

    // 创建并发送角色消失的通知消息
    Proto::RoleDisAppear disAppear;
    disAppear.set_sn(pPlayer->GetPlayerSN()); // 设置消失的角色序列号
    // 广播该角色消失的消息给所有客户端
    BroadcastPacket(Proto::MsgId::S2C_RoleDisAppear, disAppear);
}

void World::HandleMove(Player* pPlayer, Packet* pPacket)
{
    // 解析从客户端收到的移动消息
    auto proto = pPacket->ParseToProto<Proto::Move>();
    // 设置消息中的玩家序列号
    proto.set_player_sn(pPlayer->GetPlayerSN());
    // 获取位置数据
    const auto positions = proto.mutable_position();

    // 尝试获取玩家的移动组件
    auto pMoveComponent = pPlayer->GetComponent<MoveComponent>();
    if (pMoveComponent == nullptr)
    {
        // 如果不存在，则添加移动组件
        pMoveComponent = pPlayer->AddComponent<MoveComponent>();
    }

    // 创建一个队列以存储目标位置
    std::queue<Vector3> pos;
    for (auto index = 0; index < proto.position_size(); index++)
    {
        Vector3 v3(0, 0, 0);
        // 从位置数据中解析出每一个目标位置并加入队列
        v3.ParserFromProto(positions->Get(index));
        pos.push(v3);

        //LOG_DEBUG("move target. " << v3); // 可以记录每个目标位置的调试信息
    }

    // 获取玩家最后位置组件
    const auto pComponentLastMap = pPlayer->GetComponent<PlayerComponentLastMap>();
    // 更新移动组件的目标位置和当前所在位置
    pMoveComponent->Update(pos, pComponentLastMap->GetCur()->Position);

    // 广播移动消息到所有客户端
    BroadcastPacket(Proto::MsgId::S2C_Move, proto);
}
