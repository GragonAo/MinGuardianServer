#include "thread_mgr.h"
#include "common.h"
#include "message_system.h"
#include "yaml.h"
#include "log4.h"
#include "res_path.h"
#include "packet.h"
#include "log4_help.h"

#include "network_locator.h"
#include "console.h"
#include "console_cmd_thread.h"

#include "thread_collector_exclusive.h"
#include "global.h"
#include "trace_component.h"
#include "console_cmd_trace.h"
#include "component_help.h"

ThreadMgr::ThreadMgr()
{

}

void ThreadMgr::InitializeThread()
{
    // 从全局配置中读取线程配置并初始化相应类型的线程
    const auto pGlobal = Global::GetInstance();
    auto pYaml = GetEntitySystem()->GetComponent<Yaml>();
    const auto pConfig = pYaml->GetConfig(pGlobal->GetCurAppType());
    const auto pAppConfig = dynamic_cast<AppConfig*>(pConfig);

    if (pAppConfig->LogicThreadNum > 0)
    {
        CreateThread(LogicThread, pAppConfig->LogicThreadNum);  // 创建逻辑线程
    }

    if (pAppConfig->MysqlThreadNum > 0)
    {
        CreateThread(MysqlThread, pAppConfig->MysqlThreadNum);  // 创建数据库线程
    }

    if (pAppConfig->LogicThreadNum > 0 || pAppConfig->MysqlThreadNum > 0)
    {
        // 创建网络相关的线程，例如监听线程和连接线程
        if (pAppConfig->ListenThreadNum > 0)
            CreateThread(ListenThread, pAppConfig->ListenThreadNum);

        if (pAppConfig->ConnectThreadNum > 0)
            CreateThread(ConnectThread, pAppConfig->ConnectThreadNum);
    }
}

void ThreadMgr::CreateThread(ThreadType iType, int num)
{
    // 根据线程类型和数量创建线程
    auto pGlobal = Global::GetInstance();
    auto pYaml = GetEntitySystem()->GetComponent<Yaml>();
    const auto pConfig = pYaml->GetConfig(pGlobal->GetCurAppType());
    auto pAppCofig = dynamic_cast<AppConfig*>(pConfig);

    if (pAppCofig->LogicThreadNum == 0 && pAppCofig->MysqlThreadNum == 0)
        return;

    LOG_DEBUG("Initialize thread:" << GetThreadTypeName(iType) << " thread num:" << num);

    // 检查是否已经存在该类型的线程管理器，如果不存在则创建新的
    auto iter = _threads.find(iType);
    if (iter == _threads.end())
    {
        if (iType == MysqlThread)
            _threads[iType] = new ThreadCollectorExclusive(iType, num);  // 为数据库线程创建专用的线程管理器
        else
            _threads[iType] = new ThreadCollector(iType, num);  // 为其他类型的线程创建通用的线程管理器
    }
    else
    {
        _threads[iType]->CreateThread(num);  // 如果已存在，增加线程数量
    }
}

void ThreadMgr::InitializeGlobalComponent(APP_TYPE ppType, int appId)
{
    // 初始化全局组件，如日志系统、配置解析器、资源路径等
    GetEntitySystem()->AddComponent<ResPath>();
    GetEntitySystem()->AddComponent<Log4>(ppType);
    GetEntitySystem()->AddComponent<Yaml>();
    GetEntitySystem()->AddComponent<NetworkLocator>();

    auto pConsole = GetEntitySystem()->AddComponent<Console>();
    pConsole->Register<ConsoleCmdThread>("thread");  // 注册命令行指令 "thread"

#ifdef LOG_TRACE_COMPONENT_OPEN
    GetEntitySystem()->AddComponent<TraceComponent>();  // 如果启用了追踪组件，进行初始化
    pConsole->Register<ConsoleCmdTrace>("trace");  // 注册追踪命令
#endif

    // 初始化主线程组件
    InitComponent(ThreadType::MainThread);
}

void ThreadMgr::Update()
{
    // 更新创建消息包和分发消息包的状态，并调用基类的更新方法
    UpdateCreatePacket();
    UpdateDispatchPacket();
    SystemManager::Update();
}

void ThreadMgr::UpdateCreatePacket()
{
    // 锁定创建包的访问，确保线程安全
    _create_lock.lock();
    if (_createPackets.CanSwap()) {
        _createPackets.Swap();  // 如果可以交换包队列，进行交换
    }
    _create_lock.unlock();

    // 获取待处理的消息包列表
    auto pList = _createPackets.GetReaderCache();
    for (auto iter = pList->begin(); iter != pList->end(); ++iter)
    {
        auto pPacket = (*iter);
        if (_threads.size() > 0)
        {
            auto pCreateProto = pPacket->ParseToProto<Proto::CreateComponent>();
            auto threadType = (ThreadType)(pCreateProto.thread_type());

            // 根据消息包中的线程类型分发给对应的线程管理器
            if (_threads.find(threadType) == _threads.end())
            {
                LOG_ERROR("can't find threadtype:" << GetThreadTypeName(threadType));
                continue;
            }

            auto pThreadCollector = _threads[threadType];
            pThreadCollector->HandlerCreateMessage(pPacket, pCreateProto.is_to_all_thread());  // 分发创建消息
        }
        else
        {
            // 如果没有线程管理器，则将消息包添加到全局消息系统中
            GetMessageSystem()->AddPacketToList(pPacket);
        }

        pPacket->OpenRef();  // 处理完消息包后释放引用
    }
    pList->clear();  // 清空已处理的消息包列表
}

void ThreadMgr::UpdateDispatchPacket()
{
    // 锁定消息包队列，确保线程安全
    _packet_lock.lock();
    if (_packets.CanSwap()) {
        _packets.Swap();  // 如果可以交换包队列，进行交换
    }
    _packet_lock.unlock();

    // 获取待处理的消息包列表
    auto pList = _packets.GetReaderCache();
    for (auto iter = pList->begin(); iter != pList->end(); ++iter)
    {
        auto pPacket = (*iter);

        // 将消息包添加到全局消息系统和所有线程中
        GetMessageSystem()->AddPacketToList(pPacket);

        for (auto iter = _threads.begin(); iter != _threads.end(); ++iter)
        {
            iter->second->HandlerMessage(pPacket);  // 分发消息包到各线程
        }

        pPacket->OpenRef();  // 处理完消息包后释放引用
    }
    pList->clear();  // 清空已处理的消息包列表
}

bool ThreadMgr::IsStopAll()
{
    // 检查所有线程是否都已经停止
    for (auto iter = _threads.begin(); iter != _threads.end(); ++iter)
    {
        if (!iter->second->IsStopAll())
        {
            return false;  // 如果有线程未停止，返回 false
        }
    }
    return true;  // 所有线程已停止，返回 true
}

void ThreadMgr::DestroyThread()
{
    // 调用所有线程管理器的销毁函数
    for (auto iter = _threads.begin(); iter != _threads.end(); ++iter)
    {
        iter->second->DestroyThread();
    }
}

bool ThreadMgr::IsDestroyAll()
{
    // 检查所有线程是否都已经销毁
    for (auto iter = _threads.begin(); iter != _threads.end(); ++iter)
    {
        if (!iter->second->IsDestroyAll())
        {
            return false;  // 如果有线程未销毁，返回 false
        }
    }
    return true;  // 所有线程已销毁，返回 true
}

void ThreadMgr::Dispose()
{
    // 销毁所有线程管理器，并清理资源
    SystemManager::Dispose();

    for (auto iter = _threads.begin(); iter != _threads.end(); ++iter)
    {
        auto pObj = iter->second;
        pObj->Dispose();  // 调用 Dispose 方法
        delete pObj;  // 释放内存
    }

    _threads.clear();  // 清空线程管理器列表
}

void ThreadMgr::DispatchPacket(Packet* pPacket)
{
    // 将消息包添加到队列中等待分发
    std::lock_guard<std::mutex> guard(_packet_lock);
    _packets.GetWriterCache()->emplace_back(pPacket);
}
