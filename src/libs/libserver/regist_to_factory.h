#pragma once
#include <typeinfo>

#include "component_factory.h"
#include "object_pool.h"
#include "object_pool_collector.h"
// 注册到工厂的模板类
template<typename T, typename...Targs>
class RegistToFactory
{
public:
    RegistToFactory()
    {
        // 在组件工厂中注册组件类型 T，并指定创建组件的方法
        ComponentFactory<Targs...>::GetInstance()->Regist(typeid(T).name(), CreateComponent);
    }

    // 创建组件的静态方法
    static T* CreateComponent(SystemManager* pSysMgr, uint64 sn, Targs... args)
    {
        // 从系统管理器中获取对象池收集器
        auto pCollector = pSysMgr->GetPoolCollector();
        // 动态获取对应类型的对象池
        auto pPool = dynamic_cast<DynamicObjectPool<T>*>(pCollector->GetPool<T>());
        // 从对象池中分配对象，并使用传入的参数进行初始化
        return pPool->MallocObject(pSysMgr, nullptr, sn, std::forward<Targs>(args)...);
    }
};

// 注册对象的模板类
template<typename T, typename...Targs>
class RegistObject
{
public:
    RegistObject()
    {
        // 在组件工厂中注册组件类型 T，并指定创建组件的方法
        ComponentFactory<Targs...>::GetInstance()->Regist(typeid(T).name(), CreateComponent);
    }

    // 创建组件的静态方法
    static T* CreateComponent(SystemManager* pSysMgr, uint64 sn, Targs... args)
    {
        // 直接使用传入的参数构造一个新的对象
        return new T(std::forward<Targs>(args)...);
    }
};
