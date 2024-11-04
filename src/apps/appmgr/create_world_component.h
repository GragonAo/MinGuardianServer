#pragma once
#include "libserver/entity.h"
#include "libserver/sync_component.h"

class Packet;

// CreateWorldComponent 类用于管理世界创建的逻辑
class CreateWorldComponent : public SyncComponent, public Entity<CreateWorldComponent>, public IAwakeSystem<>
{
public:
    // 类初始化方法
    void Awake() override;

    // 释放组件资源时调用
    void BackToPool() override;

private:
    // 请求创建一个新世界，返回状态码
    int ReqCreateWorld(int worldId);

    // 处理创建命令的消息
    void HandleCmdCreate(Packet* pPacket);
    
    // 处理应用信息同步的消息
    void HandleAppInfoSync(Packet* pPacket);

    // 处理网络断开的消息
    void HandleNetworkDisconnect(Packet* pPacket) override;

    // 处理请求世界的消息
    void HandleRequestWorld(Packet* pPacket);

    // 处理查询世界的消息
    void HandleQueryWorld(Packet* pPacket);

    // 处理创建世界的广播消息
    void HandleBroadcastCreateWorld(Packet* pPacket);

private:
    // 记录正在创建的世界，<world id, space Id>
    std::map<int, int> _creating;

    // 记录已创建的世界信息，<world id, world sn>
    std::map<int, uint64> _created;  // 已创建的世界及其相关信息

    // 记录副本信息，<world sn, world id>
    std::map<uint64, int> _dungeons;  // 副本的世界 ID
};
