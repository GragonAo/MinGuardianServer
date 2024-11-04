#pragma once

#include "libserver/redis_connector.h"

class Packet;

// RedisLogin 负责处理与登录相关的 Redis 消息和操作
class RedisLogin : public RedisConnector
{
private:
    // 注册消息处理函数
    void RegisterMsgFunction() override;

    // 处理登录 token 到 Redis 的操作
    void HandleLoginTokenToRedis(Packet* pPacket);
    
    // 处理查询账号是否在线的操作
    void HandleAccountQueryOnline(Packet* pPacket);

    // 处理同步账号在线状态到 Redis
    void HandleAccountSyncOnlineToRedis(Packet* pPacket);

    // 处理从 Redis 删除账号在线状态
    void HandleAccountDeleteOnlineToRedis(Packet* pPacket);
};
