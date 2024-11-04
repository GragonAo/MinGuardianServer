#pragma once

#include "component.h"
#include "disposable.h"
#include "component_factory.h"
#include "object_pool.h"
#include "component_collections.h"
#include "system_manager.h"
#include "log4_help.h"
#include "object_pool_collector.h"

class Packet;

// EntitySystem 类用于管理游戏实体及其组件
class EntitySystem : public IDisposable
{
public:
    friend class ConsoleThreadComponent;  // 允许 ConsoleThreadComponent 访问私有成员
    EntitySystem(SystemManager* pMgr);  // 构造函数，接受一个系统管理器
    virtual ~EntitySystem();  // 析构函数

    // 添加组件
    template <class T, typename... TArgs>
    T* AddComponent(TArgs... args);

    // 添加具有父组件的组件
    template <class T, typename... TArgs>
    T* AddComponentWithParent(IEntity* pParent, uint64 sn, TArgs... args);

    // 根据名称添加组件
    template <typename... TArgs>
    IComponent* AddComponentByName(std::string className, uint64 sn, TArgs... args);

    // 获取指定类型的组件
    template <class T>
    T* GetComponent();

    // 移除组件
    void RemoveComponent(IComponent* pObj);

    // 获取指定类型的组件集合
    template<class T>
    ComponentCollections* GetComponentCollections();

    void Update();  // 更新系统
    void Dispose() override;  // 释放资源

private:
    // 添加组件到系统
    template <class T>
    void AddComponent(T* pComponent);

    // 存储组件集合的映射，使用序列号作为键
    std::map<uint64, ComponentCollections*> _objSystems;

private:
    SystemManager* _systemManager;  // 系统管理器的指针
};

template<class T>
inline void EntitySystem::AddComponent(T* pComponent)
{
    const auto typeHashCode = pComponent->GetTypeHashCode();  // 获取组件类型的哈希值

#if LOG_SYSOBJ_OPEN
    LOG_SYSOBJ("*[sys] add obj. obj sn:" << pComponent->GetSN() << " type:" << pComponent->GetTypeName() << " thead id:" << std::this_thread::get_id());
#endif

    auto iter = _objSystems.find(typeHashCode);  // 查找组件类型
    if (iter == _objSystems.end())
    {
        _objSystems[typeHashCode] = new ComponentCollections(pComponent->GetTypeName());  // 创建新的组件集合
    }

    auto pEntities = _objSystems[typeHashCode];
    pEntities->Add(dynamic_cast<IComponent*>(pComponent));  // 将组件添加到集合中
    pComponent->SetSystemManager(_systemManager);  // 设置系统管理器
}

template <class T, typename ... TArgs>
T* EntitySystem::AddComponent(TArgs... args)
{
    return AddComponentWithParent<T>(nullptr, 0, std::forward<TArgs>(args)...);  // 调用带有父组件的添加方法
}

template <class T, typename ... TArgs>
T* EntitySystem::AddComponentWithParent(IEntity* pParent, uint64 sn, TArgs... args)
{
    auto pCollector = _systemManager->GetPoolCollector();  // 获取对象池收集器
    auto pPool = dynamic_cast<DynamicObjectPool<T>*>(pCollector->GetPool<T>());  // 获取对象池
    T* pComponent = pPool->MallocObject(_systemManager, pParent, sn, std::forward<TArgs>(args)...);  // 从池中分配对象
    if (pComponent == nullptr)
        return nullptr;  // 分配失败

    AddComponent(pComponent);  // 添加组件
    return pComponent;  // 返回分配的组件
}

template<typename ...TArgs>
inline IComponent* EntitySystem::AddComponentByName(std::string className, uint64 sn, TArgs ...args)
{
    auto pObj = ComponentFactory<TArgs...>::GetInstance()->Create(_systemManager, className, sn, std::forward<TArgs>(args)...);  // 创建组件
    if (pObj == nullptr)
        return nullptr;  // 创建失败

    IComponent* pComponent = static_cast<IComponent*>(pObj);
    AddComponent(pComponent);  // 添加组件
    return pComponent;  // 返回创建的组件
}

template <class T>
T* EntitySystem::GetComponent()
{
    const auto typeHashCode = typeid(T).hash_code();  // 获取类型的哈希值
    auto iter = _objSystems.find(typeHashCode);
    if (iter == _objSystems.end())
        return nullptr;  // 未找到组件

    return dynamic_cast<T*>(iter->second->Get());  // 返回指定类型的组件
}

template<class T>
inline ComponentCollections* EntitySystem::GetComponentCollections()
{
    const auto typeHashCode = typeid(T).hash_code();  // 获取类型的哈希值
    auto iter = _objSystems.find(typeHashCode);
    if (iter == _objSystems.end())
    {
        //LOG_WARN("GetComponentCollections failed. class name:" << className);
        return nullptr;  // 未找到组件集合
    }

    return iter->second;  // 返回组件集合
}
