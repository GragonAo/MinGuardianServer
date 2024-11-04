#include "component.h"
#include "entity.h"
#include "system_manager.h"
#include "thread_mgr.h"
#include "object_pool.h"
#include "timer_component.h"

// 设置组件的父实体
void IComponent::SetParent(IEntity* pObj)
{
    _parent = pObj; // 将父实体指针存储在成员变量 _parent 中
}

// 设置系统管理器
void IComponent::SetSystemManager(SystemManager* pObj)
{
    _pSystemManager = pObj; // 将系统管理器指针存储在成员变量 _pSystemManager 中
}

// 设置动态对象池
void IComponent::SetPool(IDynamicObjectPool* pPool)
{
    _pPool = pPool; // 将对象池指针存储在成员变量 _pPool 中
}

// 获取父实体
IEntity* IComponent::GetParent() const
{
    return _parent; // 返回存储的父实体指针
}

// 获取系统管理器
SystemManager* IComponent::GetSystemManager() const
{
    return _pSystemManager; // 返回存储的系统管理器指针
}

// 将组件返回到对象池
void IComponent::ComponentBackToPool()
{
    BackToPool(); // 调用基类的 BackToPool 方法

    // 检查是否有计时器
    if (!_timers.empty()) 
    {
        // 获取 TimerComponent 实例
        auto pTimer = _pSystemManager->GetEntitySystem()->GetComponent<TimerComponent>();
        if (pTimer != nullptr)
            pTimer->Remove(_timers); // 从 TimerComponent 中移除计时器

        _timers.clear(); // 清空计时器列表
    }

    // 如果对象池指针有效，释放对象
    if (_pPool != nullptr)
    {
        _pPool->FreeObject(this); // 调用对象池的 FreeObject 方法
        _pPool = nullptr; // 将对象池指针置空
    }

    _sn = 0; // 重置序列号
    _parent = nullptr; // 将父实体指针置空
    _pSystemManager = nullptr; // 将系统管理器指针置空
}

// 添加计时器
uint64 IComponent::AddTimer(const int total, const int durations, const bool immediateDo, const int immediateDoDelaySecond, TimerHandleFunction handler)
{
    // 获取 TimerComponent 实例
    auto obj = GetSystemManager()->GetEntitySystem()->GetComponent<TimerComponent>();
    // 添加计时器并返回其标识
    const auto timer = obj->Add(total, durations, immediateDo, immediateDoDelaySecond, std::move(handler));
    _timers.push_back(timer); // 将计时器标识添加到 _timers 列表中
    return timer;
}
