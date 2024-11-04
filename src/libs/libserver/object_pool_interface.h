#pragma once

#include "disposable.h"  // 包含 IDisposable 类的声明，用于资源释放管理
#include "component.h"   // 包含 IComponent 类的声明，用于表示对象池中的对象

// IDynamicObjectPool 接口继承自 IDisposable，表示动态对象池的基础接口
class IDynamicObjectPool : public IDisposable
{
public:
    // 更新对象池状态的纯虚函数，必须在派生类中实现
    virtual void Update() = 0;

    // 释放对象的纯虚函数，必须在派生类中实现
    // @param pObj: 需要释放的对象
    virtual void FreeObject(IComponent* pObj) = 0;

    // 显示对象池状态的纯虚函数，必须在派生类中实现
    virtual void Show() = 0;
};