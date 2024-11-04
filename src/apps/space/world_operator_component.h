#pragma once  // 确保头文件只被包含一次

#include "libserver/entity.h"  // 引入实体基类
#include "libserver/system.h"  // 引入系统基类

class Packet;  // 前向声明 Packet 类，避免循环依赖

// 定义 WorldOperatorComponent 类
class WorldOperatorComponent : public Entity<WorldOperatorComponent>, public IAwakeSystem<>
{
public:
    // Awake 方法，在组件被创建后调用，用于初始化
    void Awake() override;

    // BackToPool 方法，在组件返回对象池时调用，用于清理
    void BackToPool() override;

private:
    // 处理创建世界的消息
    void HandleCreateWorld(Packet* pPacket);
};
