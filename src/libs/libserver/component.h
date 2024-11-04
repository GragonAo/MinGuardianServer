#pragma once
#include "sn_object.h" // 引入基础对象类
#include "common.h"    // 引入通用定义和常量

#include <functional>  // 引入函数对象
#include <list>       // 引入列表容器

// 前向声明
class IEntity; // 实体类
class IDynamicObjectPool; // 动态对象池接口
class SystemManager; // 系统管理器

// 定义计时器处理函数类型
using TimerHandleFunction = std::function<void(void)>;

// IComponent 接口定义
class IComponent : public SnObject
{
public:
    // 声明 EntitySystem 类为友元类，以便访问私有成员
    friend class EntitySystem;

    // 默认虚构造函数
    virtual ~IComponent() = default;

    // 设置动态对象池
    void SetPool(IDynamicObjectPool* pPool);
    
    // 设置父实体
    void SetParent(IEntity* pObj);
    
    // 设置系统管理器
    void SetSystemManager(SystemManager* pObj);

    // 获取父实体（模板函数，支持类型转换）
    template<class T>
    T* GetParent();

    // 获取父实体（返回 IEntity 指针）
    IEntity* GetParent() const;

    // 获取系统管理器（返回指针）
    SystemManager* GetSystemManager() const;

    // 释放组件资源，返回池
    virtual void BackToPool() = 0;

    // 组件返回池的实现
    virtual void ComponentBackToPool();

    // 获取组件类型名称（纯虚函数）
    virtual const char* GetTypeName() = 0;

    // 获取组件类型哈希值（纯虚函数）
    virtual uint64 GetTypeHashCode() = 0;

protected:
    // 添加计时器，支持立即执行
    uint64 AddTimer(const int total, const int durations, const bool immediateDo, const int immediateDoDelaySecond, TimerHandleFunction handler);
    
    // 存储计时器的列表
    std::list<uint64> _timers;

    // 父实体指针
    IEntity* _parent{ nullptr };
    
    // 系统管理器指针
    SystemManager* _pSystemManager{ nullptr };
    
    // 动态对象池指针
    IDynamicObjectPool* _pPool{ nullptr };
};

// 从父类获取父实体的实现
template <class T>
T* IComponent::GetParent()
{
    // 使用 dynamic_cast 尝试将 _parent 转换为目标类型 T*
    return dynamic_cast<T*>(_parent);
}

// 组件模板类定义
template<class T>
class Component : public IComponent
{
public:
    // 重写获取类型名称
    const char* GetTypeName() override;

    // 重写获取类型哈希值
    uint64 GetTypeHashCode() override;
};

// 获取类型名称的实现
template <class T>
const char* Component<T>::GetTypeName()
{
    return typeid(T).name(); // 使用 typeid 获取类型名称
}

// 获取类型哈希值的实现
template <class T>
uint64 Component<T>::GetTypeHashCode()
{
    return typeid(T).hash_code(); // 使用 typeid 获取类型哈希值
}
