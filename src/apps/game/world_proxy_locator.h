#pragma once
#include "libserver/entity.h"

class Packet;

// 世界代理定位器，用于管理世界的注册和查询
class WorldProxyLocator : public Entity<WorldProxyLocator>, public IAwakeSystem<>
{
public:
    // 唤醒系统，进行初始化
    void Awake() override;
    // 回收资源，清理状态
    void BackToPool() override;

    // 注册世界到定位器
    void RegisterToLocator(int worldId, uint64 worldSn);
    // 移除已注册的世界
    void Remove(int worldId, uint64 worldSn);

    // 检查指定的世界序号是否存在于地下城中
    bool IsExistDungeon(uint64 worldSn);
    // 根据世界 ID 获取对应的世界序号
    uint64 GetWorldSnById(int worldId);

private:
    // 处理创建世界的广播消息
    void HandleBroadcastCreateWorld(Packet* pPacket);

private:
    std::mutex _lock; // 线程安全锁，用于保护共享数据

    // 存储公共世界的映射：<世界ID, 世界序号>
    std::map<int, uint64> _publics; // 公共世界

    // 存储世界序号的集合：<世界序号>
    std::set<uint64> _worlds;  // 仅存储世界序号
};
