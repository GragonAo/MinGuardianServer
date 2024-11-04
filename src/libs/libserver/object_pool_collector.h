#pragma once

#include <list>
#include <mutex>
#include <map>
#include "common.h"
#include "component.h"
#include "system.h"
#include "object_pool.h"

// 前向声明
class SystemManager;

// DynamicObjectPoolCollector 类用于管理动态对象池
class DynamicObjectPoolCollector : public IDisposable
{
public:
    // 构造函数，接收一个 SystemManager 指针，用于管理系统相关功能
    DynamicObjectPoolCollector(SystemManager* pSys);
    
    // 覆盖 IDisposable 接口的 Dispose 方法，释放资源
    void Dispose() override;

    // 模板函数，根据类型 T 获取相应的动态对象池
    template<class T>
    IDynamicObjectPool* GetPool();

    // 更新所有对象池
    void Update();
    
    // 显示所有对象池的信息
    void Show();

private:
    // 存储不同类型的动态对象池，键为类型的哈希值，值为对应的对象池指针
    std::map<uint64, IDynamicObjectPool*> _pools;
    
    // 指向 SystemManager 的指针，可能用于系统管理功能
    SystemManager* _pSystemManager{ nullptr };
};

// 模板函数实现
template<class T>
IDynamicObjectPool* DynamicObjectPoolCollector::GetPool()
{
    // 获取类型 T 的哈希值，作为查找的键
    const auto typeHashCode = typeid(T).hash_code();
    
    // 查找是否已经存在该类型的对象池
    auto iter = _pools.find(typeHashCode);
    if (iter != _pools.end())
    {
        // 如果找到，返回现有的对象池
        return iter->second;
    }

    // 如果不存在，创建一个新的动态对象池
    auto pPool = new DynamicObjectPool<T>();
    
    // 将新创建的对象池插入到 _pools 中
    _pools.insert(std::make_pair(typeHashCode, pPool));
    
    // 返回新创建的对象池
    return pPool;
}
