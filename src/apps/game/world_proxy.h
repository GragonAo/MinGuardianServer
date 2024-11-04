#pragma once
#include "libserver/entity.h"
#include "libserver/socket_object.h"
#include "libplayer/world_base.h"

class Player;

// 世界代理类，用于管理和处理与世界相关的操作
class WorldProxy : public IWorld, public Entity<WorldProxy>, public IAwakeFromPoolSystem<int, uint64>
{
public:
    // 唤醒系统，初始化世界代理
    void Awake(int worldId, uint64 lastWorldSn) override;

    // 回收资源，清理状态
    void BackToPool() override;

    // 向世界发送消息包，携带玩家信息
    void SendPacketToWorld(const Proto::MsgId msgId, ::google::protobuf::Message& proto, Player* pPlayer) const;

    // 向世界发送消息包，仅携带玩家信息
    void SendPacketToWorld(const Proto::MsgId msgId, Player* pPlayer) const;

    // 将包内容复制到世界中
    void CopyPacketToWorld(Player* pPlayer, Packet* pPacket) const;

private:
    // 根据网络标识获取玩家对象
    Player* GetPlayer(NetIdentify* pIdentify);

    // 处理网络断开连接
    void HandleNetworkDisconnect(Packet* pPacket);

    // 处理传送请求
    void HandleTeleport(Packet* pPacket);
    
    // 处理传送后的操作
    void HandleTeleportAfter(Player* pPlayer, Packet* pPacket);
    
    // 处理创建世界代理的广播消息
    void HandleBroadcastCreateWorldProxy(Packet* pPacket);
    
    // 处理玩家进入世界的请求
    void HandleC2GEnterWorld(Player* pPlayer, Packet* pPacket);
    
    // 处理同步玩家状态的消息
    void HandleS2GSyncPlayer(Player* pPlayer, Packet* pPacket);

    // 默认消息处理函数
    void HandleDefaultFunction(Packet* pPacket);

private:
    int _spaceAppId{ 0 }; // 空间应用的ID
};
