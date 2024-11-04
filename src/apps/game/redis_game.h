#pragma once
#include "libserver/redis_connector.h"

class Packet;

class RedisGame : public RedisConnector
{
private:
    // 注册消息处理函数，在子类中重写该函数来处理特定消息
    void RegisterMsgFunction() override;

    // 处理玩家上线同步到 Redis 的消息
    void HandlePlayerSyncOnlineToRedis(Packet* pPacket);

    // 处理玩家从 Redis 中删除在线状态的消息
    void HandlePlayerDeleteOnlineToRedis(Packet* pPacket);

    // 处理游戏 Token 同步到 Redis 的消息
    void HandleGameTokenToRedis(Packet* pPacket);
};
