#include "player_component_onlineinlogin.h"
#include "libserver/redis_constants.h" // 包含 Redis 相关常量
#include "libplayer/player.h" // 包含玩家相关的头文件
#include "libserver/message_system_help.h" // 包含消息系统帮助类

// 当组件被唤醒时调用，初始化玩家账号并设置定时器
void PlayerComponentOnlineInLogin::Awake(std::string account)
{
    // 保存玩家账号
    _account = account;

    // 添加定时器，每隔一段时间同步玩家在线状态到 Redis
    // 这里的 RedisKeyAccountOnlineLoginTimeout 表示超时时间的一半，定时重复执行
    AddTimer(0, (int)(RedisKeyAccountOnlineLoginTimeout*0.5), true, 0, BindFunP0(this, &PlayerComponentOnlineInLogin::SetOnlineFlag));
}

// 当组件回收到对象池时调用，通知 Redis 删除该玩家的在线状态
void PlayerComponentOnlineInLogin::BackToPool()
{
    // 创建用于通知 Redis 删除在线信息的 Proto 消息
    Proto::AccountDeleteOnlineToRedis protoSync;
    protoSync.set_account(_account.c_str());

    // 发送删除在线状态的消息到 Redis
    MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_AccountDeleteOnlineToRedis, protoSync, nullptr);
}

// 设置玩家在线状态标志，将其同步到 Redis
void PlayerComponentOnlineInLogin::SetOnlineFlag() const
{
    // 创建用于同步在线信息的 Proto 消息
    Proto::AccountSyncOnlineToRedis protoSync;
    protoSync.set_account(_account.c_str());

    // 发送在线状态同步消息到 Redis
    MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_AccountSyncOnlineToRedis, protoSync, nullptr);
}
