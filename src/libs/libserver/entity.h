#pragma once

#include <map>
#include <list>
#include <memory>
#include <algorithm>
#include <queue>

#include "component.h"
#include "entity_system.h"

// IEntity类继承自IComponent，表示实体对象基类
class IEntity : public IComponent
{
public:
    virtual ~IEntity() = default;

    // 将组件返回到对象池，清理资源
    void ComponentBackToPool() override;

    // 添加一个组件，自动生成序列号
    template <class T, typename... TArgs>
    T* AddComponent(TArgs... args);

    // 带有特定序列号的添加组件方法
    template <class T, typename... TArgs>
    T* AddComponentWithSn(uint64 sn, TArgs... args);
    
    // 获取特定类型的组件
    template<class T>
    T* GetComponent();

    // 删除特定类型的组件
    template<class T>
    void RemoveComponent();

    // 删除指定的组件实例
    void RemoveComponent(IComponent* pObj);

protected:
    // 存储实体中的组件，使用组件的类型哈希码作为键
    std::map<uint64, IComponent*> _components;
};

// 添加组件（不带序列号，默认序列号为0）
template <class T, typename... TArgs>
inline T* IEntity::AddComponent(TArgs... args)
{
    return AddComponentWithSn<T>(0, std::forward<TArgs>(args)...);
}

// 带序列号的添加组件方法
template <class T, typename... TArgs>
T* IEntity::AddComponentWithSn(uint64 sn, TArgs... args)
{
    const auto typeHashCode = typeid(T).hash_code();
    // 检查是否已经存在同类型的组件，避免重复添加
    if (_components.find(typeHashCode) != _components.end())
    {
        LOG_ERROR("Add same component. type:" << typeid(T).name());
        return nullptr;
    }

    // 创建组件并将其与实体关联
    T* pComponent = _pSystemManager->GetEntitySystem()->AddComponentWithParent<T>(this, sn, std::forward<TArgs>(args)...);
    _components.insert(std::make_pair(typeHashCode, pComponent));
    return pComponent;
}

// 获取特定类型的组件
template<class T>
T* IEntity::GetComponent()
{
    const auto typeHashCode = typeid(T).hash_code();
    const auto iter = _components.find(typeHashCode);
    if (iter == _components.end())
        return nullptr;

    return dynamic_cast<T*>(iter->second);
}

// 移除特定类型的组件
template<class T>
void IEntity::RemoveComponent()
{
    const auto typeHashCode = typeid(T).hash_code();
    const auto iter = _components.find(typeHashCode);
    if (iter == _components.end())
    {
        LOG_ERROR("Entity RemoveComponent error. not find. className:" << typeid(T).name());
        return;
    }

    auto pComponent = iter->second;
    RemoveComponent(pComponent);
}

// Entity类是一个泛型类，继承自IEntity，表示具体的实体类型
template<class T>
class Entity : virtual public IEntity
{
public:
    // 返回实体的类型名称
    const char* GetTypeName() override;

    // 返回实体的类型哈希码
    uint64 GetTypeHashCode() override;
};

// 获取实体类型的名称
template <class T>
const char* Entity<T>::GetTypeName()
{
    return typeid(T).name();
}

// 获取实体类型的哈希码
template <class T>
uint64 Entity<T>::GetTypeHashCode()
{
    return typeid(T).hash_code();
}
