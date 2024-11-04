#include "redis_login.h"
#include "libserver/protobuf/proto_id.pb.h"
#include "libserver/redis_constants.h"
#include "libserver/log4_help.h"

#include "libserver/message_system_help.h"
#include "libserver/global.h"
#include "libserver/message_system.h"

// 注册 RedisLogin 类处理的消息函数
void RedisLogin::RegisterMsgFunction()
{
    auto pMsgSystem = GetSystemManager()->GetMessageSystem();

    // 注册处理账号在线同步的消息
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_AccountSyncOnlineToRedis, BindFunP1(this, &RedisLogin::HandleAccountSyncOnlineToRedis));
    // 注册处理账号在线状态删除的消息
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_AccountDeleteOnlineToRedis, BindFunP1(this, &RedisLogin::HandleAccountDeleteOnlineToRedis));
    // 注册处理登录 token 到 Redis 的消息
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_LoginTokenToRedis, BindFunP1(this, &RedisLogin::HandleLoginTokenToRedis));
    // 注册处理查询账号在线状态的消息
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_AccountQueryOnlineToRedis, BindFunP1(this, &RedisLogin::HandleAccountQueryOnline));
}

// 处理将账号在线状态同步到 Redis 的逻辑
void RedisLogin::HandleAccountSyncOnlineToRedis(Packet* pPacket)
{
    auto proto = pPacket->ParseToProto<Proto::AccountSyncOnlineToRedis>();

    // 生成 Redis 键和值，标记账号为在线状态
    const std::string key = RedisKeyAccountOnlineLogin + proto.account();
    const std::string value = std::to_string(Global::GetInstance()->GetCurAppId());
    Setex(key, value, RedisKeyAccountOnlineLoginTimeout);  // 设置键的过期时间
}

// 处理从 Redis 删除账号在线状态的逻辑
void RedisLogin::HandleAccountDeleteOnlineToRedis(Packet* pPacket)
{
    auto proto = pPacket->ParseToProto<Proto::AccountDeleteOnlineToRedis>();
    const std::string key = RedisKeyAccountOnlineLogin + proto.account();

    // 从 Redis 删除该账号的在线状态
    Delete(key);
}

// 处理将登录 token 存储到 Redis 的逻辑
void RedisLogin::HandleLoginTokenToRedis(Packet* pPacket)
{
    auto protoToken = pPacket->ParseToProto<Proto::LoginTokenToRedis>();
    auto account = protoToken.account();
    auto playerSn = protoToken.player_sn();

    // 生成唯一的 token
    auto token = Global::GetInstance()->GenerateUUID();

    // 创建 TokenInfo 消息并设置 token 和 player_sn
    Proto::TokenInfo protoInfo;
    protoInfo.set_token(token);
    protoInfo.set_player_sn(playerSn);

    // 将 TokenInfo 序列化为字符串
    std::string tokenString;
    protoInfo.SerializeToString(&tokenString);

    const std::string key = RedisKeyAccountTokey + account;
    const int timeout = RedisKeyAccountTokeyTimeout;

    // 将 token 存储到 Redis，如果存储失败则记录错误日志
    if (!Setex(key, tokenString, timeout))
    {
        token = "";
        LOG_ERROR("account:" << account.c_str() << ". failed to set token.");
    }
    else
    {
#if LOG_REDIS_OPEN
        LOG_REDIS("account:" << account.c_str() << ". gen token:" << token.c_str());
#endif
    }

    // 返回 token 给客户端
    Proto::LoginTokenToRedisRs protoRs;
    protoRs.set_account(account.c_str());
    protoRs.set_token(token.c_str());
    MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_LoginTokenToRedisRs, protoRs, nullptr);
}

// 处理查询账号在线状态的逻辑
void RedisLogin::HandleAccountQueryOnline(Packet* pPacket)
{
    auto proto = pPacket->ParseToProto<Proto::AccountQueryOnlineToRedis>();
    auto account = proto.account();
    Proto::AccountQueryOnlineToRedisRs protoRs;
    protoRs.set_account(account.c_str());
    protoRs.set_return_code(Proto::AccountQueryOnlineToRedisRs::SOTR_Offline);

    // 检查账号是否在线
    if (!SetnxExpire(RedisKeyAccountOnlineLogin + proto.account(), Global::GetInstance()->GetCurAppId(), RedisKeyAccountOnlineLoginTimeout))
        protoRs.set_return_code(Proto::AccountQueryOnlineToRedisRs::SOTR_Online);

    // 检查 Game 在线状态
    if (GetInt(RedisKeyAccountOnlineGame + proto.account()) != 0)
        protoRs.set_return_code(Proto::AccountQueryOnlineToRedisRs::SOTR_Online);

    // 发送查询结果消息
    MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_AccountQueryOnlineToRedisRs, protoRs, nullptr);
}
