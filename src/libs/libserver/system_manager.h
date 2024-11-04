#pragma once

#include "disposable.h"
#include "system.h"
#include "common.h"
#include "thread_type.h"

#include <list>
#include <random>
#include "check_time_component.h"
#include "update_system.h"

class EntitySystem;
class MessageSystem;
class DynamicObjectPoolCollector;

// 系统管理类，负责管理各个系统和组件的生命周期
class SystemManager : virtual public IDisposable, public CheckTimeComponent
{
public:
    SystemManager(); // 构造函数，初始化系统管理器
    void InitComponent(ThreadType threadType); // 初始化组件，设置线程类型

    virtual void Update(); // 更新所有系统
    void Dispose() override; // 释放资源，重写基类的Dispose方法

    MessageSystem* GetMessageSystem() const; // 获取消息系统
    EntitySystem* GetEntitySystem() const; // 获取实体系统
    UpdateSystem* GetUpdateSystem() const; // 获取更新系统

    DynamicObjectPoolCollector* GetPoolCollector() const; // 获取对象池收集器

    std::default_random_engine* GetRandomEngine() const; // 获取随机数生成器

    void AddSystem(const std::string& name); // 添加系统

protected:
    MessageSystem* _pMessageSystem; // 消息系统指针
    EntitySystem* _pEntitySystem; // 实体系统指针
    UpdateSystem* _pUpdateSystem; // 更新系统指针

    std::list<System*> _systems; // 存储系统的列表

    std::default_random_engine* _pRandomEngine; // 随机数生成器指针

    DynamicObjectPoolCollector* _pPoolCollector; // 对象池收集器指针
};
