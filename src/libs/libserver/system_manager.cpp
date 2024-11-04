#include "system_manager.h"

#include "create_component.h"

#include "message_system.h"
#include "entity_system.h"
#include "update_system.h"
#include "console_thread_component.h"
#include "object_pool_collector.h"
#include "timer_component.h"

#include <thread>

// 系统管理器构造函数
SystemManager::SystemManager()
{
    // 创建实体系统和消息系统
    _pEntitySystem = new EntitySystem(this);
    _pMessageSystem = new MessageSystem(this);

    // 创建更新系统并添加到系统列表中
    _pUpdateSystem = new UpdateSystem();
    _systems.emplace_back(_pUpdateSystem);

    // 生成随机种子，使用线程ID生成随机数生成器
    std::stringstream strStream;
    strStream << std::this_thread::get_id();
    std::string idstr = strStream.str();
    std::seed_seq seed1(idstr.begin(), idstr.end());
    std::minstd_rand0 generator(seed1);
    _pRandomEngine = new std::default_random_engine(generator());

    // 创建对象池收集器
    _pPoolCollector = new DynamicObjectPoolCollector(this);
}

// 初始化组件
void SystemManager::InitComponent(ThreadType threadType)
{
    // 向实体系统添加计时器组件和创建组件
    _pEntitySystem->AddComponent<TimerComponent>();
    _pEntitySystem->AddComponent<CreateComponentC>();
    _pEntitySystem->AddComponent<ConsoleThreadComponent>(threadType);
}

// 更新所有系统
void SystemManager::Update()
{
#if LOG_TRACE_COMPONENT_OPEN
    CheckBegin(); // 检查开始时间
#endif

    // 更新对象池收集器
    _pPoolCollector->Update();
#if LOG_TRACE_COMPONENT_OPEN
    CheckPoint("pool"); // 记录对象池更新检查点
#endif

    // 更新实体系统
    _pEntitySystem->Update();
#if LOG_TRACE_COMPONENT_OPEN
    CheckPoint("entity"); // 记录实体系统更新检查点
#endif

    // 更新消息系统
    _pMessageSystem->Update(_pEntitySystem);
#if LOG_TRACE_COMPONENT_OPEN
    CheckPoint("message"); // 记录消息系统更新检查点
#endif

    // 更新其他系统
    for (auto iter = _systems.begin(); iter != _systems.end(); ++iter)
    {
        auto pSys = (*iter);
        pSys->Update(_pEntitySystem);
#if LOG_TRACE_COMPONENT_OPEN
        CheckPoint(pSys->GetTypeName()); // 记录系统类型名称检查点
#endif
    }
}

// 释放资源
void SystemManager::Dispose()
{
    // 释放所有系统
    for (auto one : _systems)
    {
        one->Dispose(); // 调用Dispose方法
        delete one; // 删除系统对象
    }
    _systems.clear(); // 清空系统列表

    delete _pRandomEngine; // 删除随机数生成器
    _pRandomEngine = nullptr;

    _pMessageSystem->Dispose(); // 释放消息系统
    delete _pMessageSystem;
    _pMessageSystem = nullptr;

    _pEntitySystem->Dispose(); // 释放实体系统
    delete _pEntitySystem;
    _pEntitySystem = nullptr;

    // 更新对象池收集器以确保释放资源
    _pPoolCollector->Update();
    _pPoolCollector->Dispose(); // 释放对象池收集器
    delete _pPoolCollector;
    _pPoolCollector = nullptr;
}

// 获取随机数生成器
std::default_random_engine* SystemManager::GetRandomEngine() const
{
    return _pRandomEngine;
}

// 添加系统
void SystemManager::AddSystem(const std::string& name)
{
    // 使用组件工厂创建新系统
    auto pObj = ComponentFactory<>::GetInstance()->Create(nullptr, name, 0);
    if (pObj == nullptr)
    {
        LOG_ERROR("failed to create system."); // 创建系统失败
        return;
    }

    System* pSystem = static_cast<System*>(pObj);
    if (pSystem == nullptr)
    {
        LOG_ERROR("failed to create system."); // 创建系统失败
        return;
    }

    LOG_DEBUG("create system. name:" << name.c_str() << " thread id:" << std::this_thread::get_id());
    _systems.emplace_back(pSystem); // 将新系统添加到系统列表中
}

// 获取消息系统
MessageSystem* SystemManager::GetMessageSystem() const
{
    return _pMessageSystem;
}

// 获取实体系统
EntitySystem* SystemManager::GetEntitySystem() const
{
    return _pEntitySystem;
}

// 获取更新系统
UpdateSystem* SystemManager::GetUpdateSystem() const
{
    return _pUpdateSystem;
}

// 获取对象池收集器
DynamicObjectPoolCollector* SystemManager::GetPoolCollector() const
{
    return _pPoolCollector;
}
