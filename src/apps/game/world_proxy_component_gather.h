#pragma once
#include "libserver/system.h"
#include "libserver/entity.h"

// WorldProxyComponentGather 类负责定期同步世界的信息，包括世界的状态和在线玩家的数量。通过使用定时器，
// 该组件可以确保相关的世界信息能够及时更新到需要的系统中，从而实现良好的信息流动和管理。

class WorldProxyComponentGather : public Entity<WorldProxyComponentGather>, public IAwakeFromPoolSystem<>
{
public:
    void Awake() override; // 初始化方法
    void BackToPool() override; // 归还对象到池中的方法

private:
    void SyncWorldInfoToGather(); // 同步世界信息到聚合器

private:
    uint64 _worldSn{ 0 }; // 世界的序号
    int _worldId{ 0 };    // 世界的ID
};
