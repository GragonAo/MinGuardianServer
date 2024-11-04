#include "object_pool_collector.h"
#include "system_manager.h"

// 构造函数，接收一个 SystemManager 指针并初始化 _pSystemManager
DynamicObjectPoolCollector::DynamicObjectPoolCollector(SystemManager* pSys)
{
    _pSystemManager = pSys;  // 保存传入的 SystemManager 指针
}

// 释放动态对象池中的资源
void DynamicObjectPoolCollector::Dispose()
{
    // 遍历 _pools 中的每一个对象池
    for (auto iter = _pools.begin(); iter != _pools.end(); ++iter)
    {
        auto pObj = iter->second;  // 获取对象池指针
        pObj->Dispose();            // 调用对象池的 Dispose 方法，释放对象池管理的资源
        delete pObj;                // 释放对象池本身的内存
    }

    // 清空 _pools 容器
    _pools.clear();
}

// 更新所有动态对象池的状态
void DynamicObjectPoolCollector::Update()
{
    // 遍历 _pools 中的每一个对象池，调用它们的 Update 方法
    for (auto iter = _pools.begin(); iter != _pools.end(); ++iter)
    {
        iter->second->Update();  // 更新当前对象池
    }
}

// 显示所有动态对象池的信息
void DynamicObjectPoolCollector::Show()
{
    // 遍历 _pools 中的每一个对象池，调用它们的 Show 方法
    for (auto iter = _pools.begin(); iter != _pools.end(); ++iter)
    {
        iter->second->Show();  // 显示当前对象池的信息
    }
}
