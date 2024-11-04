#include "entity.h"
#include "entity_system.h"

void IEntity::ComponentBackToPool()
{
    auto pSystemManager = GetSystemManager();  // 获取系统管理器
    for (auto pair : _components)  // 遍历实体中的所有组件
    {
        // 将组件从 EntitySystem 中移除
        if (pSystemManager != nullptr)
            pSystemManager->GetEntitySystem()->RemoveComponent(pair.second);  // 从 EntitySystem 中移除组件
        else
            pair.second->ComponentBackToPool();  // 如果没有系统管理器，则直接回收组件
    }

    _components.clear();  // 清空组件映射

    IComponent::ComponentBackToPool();  // 回收实体自身
}

void IEntity::RemoveComponent(IComponent* pComponent)
{
    const auto typeHashCode = pComponent->GetTypeHashCode();  // 获取组件的类型哈希值
    _components.erase(typeHashCode);  // 从组件映射表中移除该组件
    auto pSystemManager = GetSystemManager();  // 获取系统管理器
    if (pSystemManager == nullptr)
    {
        pComponent->ComponentBackToPool();  // 如果没有系统管理器，直接回收组件
    }
    else
    {
        // 将组件从 EntitySystem 中移除
        pSystemManager->GetEntitySystem()->RemoveComponent(pComponent);
    }
}
