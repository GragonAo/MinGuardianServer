#include "player_component_onlinegame.h"
#include "libserver/redis_constants.h"

#include "libplayer/player.h"
#include "libserver/entity.h"
#include "libserver/message_system_help.h"

// 玩家在线状态组件实现
void PlayerComponentOnlineInGame::Awake(const std::string account, int version)
{
    // 初始化玩家账号和在线版本，并设置为未从数据库读取
    _account = account;
    _onlineVersion = version;
    _isReadFromeDB = false;

    // 添加定时器，每隔一段时间设置在线标志
    AddTimer(0, (int)(RedisKeyAccountOnlineGameTimeout * 0.5), true, 0, BindFunP0(this, &PlayerComponentOnlineInGame::SetOnlineFlag));
}

void PlayerComponentOnlineInGame::Awake(const std::string account)
{
    // 初始化玩家账号，在线版本设置为 0，并设置为从数据库读取
    _account = account;
    _onlineVersion = 0;
    _isReadFromeDB = true;

    // 从父对象获取玩家指针并解析玩家的 Proto 信息
	Player* pPlayer = dynamic_cast<Player*>(_parent);
    ParserFromProto(pPlayer->GetPlayerProto());
    _onlineVersion++;

    // 添加定时器，每隔一段时间设置在线标志
    AddTimer(0, (int)(RedisKeyAccountOnlineGameTimeout * 0.5), true, 0, BindFunP0(this, &PlayerComponentOnlineInGame::SetOnlineFlag));
}

void PlayerComponentOnlineInGame::BackToPool()
{
    // 当组件归还到池中时，向 Redis 发送玩家删除在线信息
    Proto::PlayerDeleteOnlineToRedis protoSync;
    protoSync.set_account(_account.c_str());
    protoSync.set_version(_onlineVersion);
    MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_PlayerDeleteOnlineToRedis, protoSync, nullptr);
}

void PlayerComponentOnlineInGame::SetOnlineFlag() const
{
    // 发送在线标志同步信息到 Redis
    Proto::PlayerSyncOnlineToRedis protoSync;
    protoSync.set_account(_account.c_str());
    protoSync.set_version(_onlineVersion);
    MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_PlayerSyncOnlineToRedis, protoSync, nullptr);
}

void PlayerComponentOnlineInGame::ParserFromProto(const Proto::Player& proto)
{
    // 如果未从数据库读取，则不进行处理
    if (!_isReadFromeDB)
        return;

    // 更新在线版本，如果 Proto 中的版本大于当前版本，则更新
    const auto version = proto.misc().online_version();
    if (_onlineVersion < version)
        _onlineVersion = version;
}

void PlayerComponentOnlineInGame::SerializeToProto(Proto::Player* pProto)
{
    // 将在线版本信息序列化到 Proto 中
    pProto->mutable_misc()->set_online_version(_onlineVersion);
}
