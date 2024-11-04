#pragma once
#include "sn_object.h"
#include <string>
#include <functional>
#include <map>
#include <iostream>
#include <mutex>
#include "system_manager.h"

template<typename ...Targs>
class ComponentFactory
{
public:
    typedef std::function<SnObject*(SystemManager*, uint64 sn, Targs...)> FactoryFunction;  // 定义工厂函数类型

    // 获取单例实例
    static ComponentFactory<Targs...>* GetInstance()
    {
        if (_pInstance == nullptr)
        {
            _pInstance = new ComponentFactory<Targs...>();  // 创建实例
        }
        return _pInstance;  // 返回实例
    }

    // 注册组件工厂函数
    bool Regist(const std::string & className, FactoryFunction pFunc)
    {
        std::lock_guard<std::mutex> guard(_lock);  // 加锁以确保线程安全
        if (_map.find(className) != _map.end())
            return false;  // 如果已经注册，返回 false

        _map.insert(std::make_pair(className, pFunc));  // 注册工厂函数
        return true;  // 返回成功
    }

    // 检查组件是否已注册
    bool IsRegisted(const std::string & className)
    {
        std::lock_guard<std::mutex> guard(_lock);  // 加锁以确保线程安全
        return _map.find(className) != _map.end();  // 返回注册状态
    }

    // 创建组件实例
    SnObject* Create(SystemManager* pSysMgr, const std::string className, uint64 sn, Targs... args)
    {
        _lock.lock();  // 手动加锁
        auto iter = _map.find(className);  // 查找组件
        if (iter == _map.end())
        {
            std::cout << "ComponentFactory Create failed. can't find component. className:" << className.c_str() << std::endl;
            return nullptr;  // 如果未找到，返回 nullptr
        }
        auto fun = iter->second;  // 获取工厂函数
        _lock.unlock();  // 解锁

        return fun(pSysMgr, sn, std::forward<Targs>(args)...);  // 调用工厂函数创建组件
    }

private:
    static ComponentFactory<Targs...>* _pInstance;  // 单例实例

    std::map<std::string, FactoryFunction> _map;  // 组件注册表

    std::mutex _lock;  // 线程安全锁
};

// 静态成员初始化
template<typename ...Targs>
ComponentFactory<Targs...>* ComponentFactory<Targs...>::_pInstance = nullptr;
