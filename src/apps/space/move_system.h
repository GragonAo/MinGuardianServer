#pragma once // 确保该头文件只被包含一次

#include "libserver/system.h"                  // 包含系统基类的头文件
#include "libserver/util_time.h"               // 包含时间工具类的头文件
#include "libserver/component_collections.h"    // 包含组件集合管理类的头文件

// MoveSystem 类声明，继承自 ISystem 接口
class MoveSystem : public ISystem<MoveSystem>
{
public:
    MoveSystem(); // 构造函数声明
    void Update(EntitySystem* pEntities) override; // 更新方法声明，覆盖基类的虚函数

private:
    timeutil::Time _lastTime; // 用于记录上一次更新时间的时间对象
    ComponentCollections* _pCollections{ nullptr }; // 指向组件集合的指针，初始化为 nullptr
};
