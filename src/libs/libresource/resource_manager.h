#pragma once

#include "libserver/entity.h" // 引入基础实体类
#include "resource_world.h"    // 引入资源管理类

// 资源管理器类
class ResourceManager : public Entity<ResourceManager>, public IAwakeSystem<>
{
public:
    // 初始化方法
    void Awake() override;
    // 返回对象池的方法
    void BackToPool() override;

public:
    ResourceWorldMgr* Worlds; // 指向资源世界管理器的指针
};
