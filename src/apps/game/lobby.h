#pragma once
#include "libplayer/world_base.h"
#include "libserver/system.h"
#include "libserver/entity.h"
#include "libserver/socket_object.h"

class Packet;
class Player;

// Lobby 类用于处理玩家的连接、登录和世界相关操作
class Lobby : public Entity<Lobby>, public IWorld, public IAwakeSystem<>
{
public:
    // 类的初始化方法
    void Awake() override;
    
    // 类的回收方法
    void BackToPool() override;

private:
    // 根据 NetIdentify 获取对应的玩家
    Player* GetPlayer(NetIdentify* pIdentify);

    // 处理网络断开连接
    void HandleNetworkDisconnect(Packet* pPacket);
    
    // 处理通过令牌的登录
    void HandleLoginByToken(Packet* pPacket);
    
    // 处理游戏令牌的 Redis 返回结果
    void HandleGameTokenToRedisRs(Packet* pPacket);
    
    // 处理查询玩家的返回结果
    void HandleQueryPlayerRs(Packet* pPacket);
    
    // 处理查询世界的返回结果
    void HandleQueryWorldRs(Packet* pPacket);
    
    // 处理广播创建世界代理的请求
    void HandleBroadcastCreateWorldProxy(Packet* pPacket);
    
    // 处理玩家传送后的逻辑
    void HandleTeleportAfter(Player* pPlayer, Packet* pPacket);

    // 进入公共世界的逻辑
    void EnterPublicWorld(Player* pPlayer);

private:
    // 等待进入世界的玩家（以世界 ID 作为键，值为等待的玩家 ID 集合）
    std::map<int, std::set<uint64>> _waitingForWorld;
    
    // 等待进入副本的玩家（以副本 ID 作为键，值为等待的玩家 ID 集合）
    std::map<uint64, std::set<uint64>> _waitingForDungeon;
};
