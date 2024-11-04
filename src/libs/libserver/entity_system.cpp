#include "entity_system.h"
#include "log4.h"
#include "console.h"

EntitySystem::EntitySystem(SystemManager* pMgr)
{
    _systemManager = pMgr;
}

EntitySystem::~EntitySystem()
{
}

void EntitySystem::RemoveComponent(IComponent* pObj)
{
    const auto entitySn = pObj->GetSN();  // 获取组件的唯一标识符 (SN)
    const auto typeHashCode = pObj->GetTypeHashCode();  // 获取组件类型的哈希值

    // 根据组件的类型哈希值查找对应的组件集合
    const auto iterObj = _objSystems.find(typeHashCode);
    if (iterObj == _objSystems.end())  // 如果找不到该类型的组件集合
    {
        LOG_WARN("destroy class failed. class's name:" << pObj->GetTypeName() << " . not found class.");
        return;
    }

    // 获取对应的组件集合
    ComponentCollections* pCollector = iterObj->second;
    pCollector->Remove(entitySn);  // 从组件集合中移除指定的组件

#if LOG_SYSOBJ_OPEN
    LOG_SYSOBJ("*[sys] remove obj. obj sn:" << pObj->GetSN() << " type:" << pObj->GetTypeName() << " thead id:" << std::this_thread::get_id());
#endif
}

void EntitySystem::Update()
{
    // 更新所有组件集合，将暂存的组件添加到主集合中，删除标记的组件
    for (auto iter : _objSystems)
    {
        iter.second->Swap();
    }
}

void EntitySystem::Dispose()
{
    // 释放所有组件的资源
    // 1. 调用每个组件集合的Dispose方法，释放集合中的组件
    // 2. 删除组件集合对象自身
    for (const auto one : _objSystems)
    {
        auto pCollections = one.second;
        pCollections->Dispose();  // 释放组件集合中的组件
    }
    
    for (auto one : _objSystems) 
    {       
        delete one.second;  // 删除组件集合对象
    }
    _objSystems.clear();  // 清空组件系统的映射表
}
