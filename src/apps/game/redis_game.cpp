#include "redis_game.h"
#include "libserver/redis_constants.h"
#include "libserver/message_system_help.h"
#include "libserver/message_system.h"

void RedisGame::RegisterMsgFunction()
{
    // 获取消息系统实例，并为特定消息 ID 注册处理函数
    auto pMsgSystem = GetSystemManager()->GetMessageSystem();

    // 注册玩家上线同步到 Redis 的消息处理函数
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_PlayerSyncOnlineToRedis, BindFunP1(this, &RedisGame::HandlePlayerSyncOnlineToRedis));

    // 注册玩家从 Redis 删除在线状态的消息处理函数
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_PlayerDeleteOnlineToRedis, BindFunP1(this, &RedisGame::HandlePlayerDeleteOnlineToRedis));

    // 注册游戏 Token 同步到 Redis 的消息处理函数
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_GameTokenToRedis, BindFunP1(this, &RedisGame::HandleGameTokenToRedis));
}

void RedisGame::HandlePlayerSyncOnlineToRedis(Packet* pPacket)
{
    // 解析玩家上线同步消息的 Protobuf 数据
    auto proto = pPacket->ParseToProto<Proto::PlayerSyncOnlineToRedis>();
    auto curValue = proto.version();  // 获取当前版本号

    // 构建 Redis 存储键值
    const std::string key = RedisKeyAccountOnlineGame + proto.account();
    const auto onlineVersion = this->GetInt(key);  // 获取当前在线版本号

    // 如果当前版本号小于在线版本号，则不进行更新
    if (curValue < onlineVersion)
        return;

    // 将玩家在线信息存入 Redis，并设置过期时间
    Setex(key, curValue, RedisKeyAccountOnlineGameTimeout);
}

void RedisGame::HandlePlayerDeleteOnlineToRedis(Packet* pPacket)
{
    // 解析玩家删除在线状态的 Protobuf 数据
    auto proto = pPacket->ParseToProto<Proto::PlayerDeleteOnlineToRedis>();
    auto curValue = proto.version();  // 获取当前版本号

    // 构建 Redis 存储键值
    const std::string key = RedisKeyAccountOnlineGame + proto.account();
    const auto onlineVersion = this->GetInt(key);  // 获取当前在线版本号

    // 如果当前版本号小于在线版本号，则不进行删除操作
    if (curValue < onlineVersion)
        return;

    // 从 Redis 中删除玩家在线信息
    Delete(key);
}

void RedisGame::HandleGameTokenToRedis(Packet* pPacket)
{
    // 解析游戏 Token 同步到 Redis 的 Protobuf 数据
    auto protoToken = pPacket->ParseToProto<Proto::GameTokenToRedis>();

    // 构建返回消息的 Protobuf 对象
    Proto::GameTokenToRedisRs protoRs;
    protoRs.set_account(protoToken.account().c_str());  // 设置账号信息

    // 从 Redis 中获取对应账号的 Token 信息
    const std::string tokenValue = GetString(RedisKeyAccountTokey + protoToken.account());
    protoRs.mutable_token_info()->ParseFromString(tokenValue);  // 将 Token 信息解析为 Protobuf

    // 从 Redis 中删除该账号的 Token 信息
    Delete(RedisKeyAccountTokey + protoToken.account());

    // 发送处理后的消息返回结果
    MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_GameTokenToRedisRs, protoRs, nullptr);
}
