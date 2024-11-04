#include "console_thread_component.h"

#include "create_component.h"
#include "update_component.h"
#include "entity_system.h"

#include <thread>
#include "network_listen.h"
#include "util_string.h"
#include "timer_component.h"

// 静态互斥锁，用于线程安全的日志输出
std::mutex ConsoleThreadComponent::_show_lock;

void ConsoleThreadComponent::Awake(ThreadType iType)
{
    _threadType = iType; // 保存线程类型

    auto pMsgSystem = GetSystemManager()->GetMessageSystem(); // 创建新的消息回调函数
    // 注册命令线程处理函数
    pMsgSystem->RegisterFunction(this,Proto::MsgId::MI_CmdThread, BindFunP1(this, &ConsoleThreadComponent::HandleCmdThread));
}

void ConsoleThreadComponent::BackToPool()
{
    // 清理逻辑（如果需要）
}

void ConsoleThreadComponent::HandleCmdThread(Packet* pPacket)
{
    // 从数据包中解析出命令
    auto cmdProto = pPacket->ParseToProto<Proto::CmdThread>();
    auto cmdType = cmdProto.cmd_type();

    // 根据命令类型处理相应的逻辑
    if (cmdType == Proto::CmdThread_CmdType_Entity)
        HandleCmdThreadEntities(pPacket);
    else if (cmdType == Proto::CmdThread_CmdType_Connect)
        HandleCmdThreadConnect(pPacket);
    else
        HandleCmdThreadPool(pPacket);
}

void ConsoleThreadComponent::HandleCmdThreadPool(Packet* pPacket)
{
    std::lock_guard<std::mutex> guard(_show_lock); // 使用互斥锁，确保线程安全

    // 输出线程信息
    LOG_DEBUG("------------------------------------");
    LOG_DEBUG(" 线程 ID:" << std::this_thread::get_id());
    LOG_DEBUG(" 线程类型:" << GetThreadTypeName(_threadType));

    // 显示线程池信息
    auto pPool = GetSystemManager()->GetPoolCollector();
    pPool->Show();
}

void ConsoleThreadComponent::HandleCmdThreadConnect(Packet* pPacket)
{
    std::lock_guard<std::mutex> guard(_show_lock); // 使用互斥锁，确保线程安全
    const auto pNetworkListen = GetSystemManager()->GetEntitySystem()->GetComponent<NetworkListen>();
    if (pNetworkListen == nullptr)
        return; // 如果没有网络监听器，直接返回

    // 输出网络连接信息
    LOG_DEBUG("------------------------------------");
    LOG_DEBUG(" 网络类型:" << GetNetworkTypeName(pNetworkListen->GetNetworkType()));
    pNetworkListen->CmdShow(); // 显示网络监听器的命令
}

void ConsoleThreadComponent::HandleCmdThreadEntities(Packet* pPacket)
{
    std::lock_guard<std::mutex> guard(_show_lock); // 使用互斥锁，确保线程安全

    // 排除不需要显示的实体类型
    std::list<uint64> excludes;
    excludes.push_back(typeid(CreateComponentC).hash_code());
    excludes.push_back(typeid(UpdateComponent).hash_code());
    excludes.push_back(typeid(ConsoleThreadComponent).hash_code());
    excludes.push_back(typeid(TimerComponent).hash_code());

    // 输出线程信息
    LOG_DEBUG("------------------------------------");
    LOG_DEBUG(" 线程 ID:" << std::this_thread::get_id());
    LOG_DEBUG(" 线程类型:" << GetThreadTypeName(_threadType));

    NetworkListen* pListen = nullptr; // 用于存储网络监听器的指针
    auto collects = GetSystemManager()->GetEntitySystem()->_objSystems; // 获取所有系统
    int total = 0; // 实体的总数量

    // 遍历所有系统集合
    for (const auto one : collects)
    {
        // 跳过不需要显示的实体类型
        if (std::find(excludes.begin(), excludes.end(), one.first) != excludes.end())
            continue;

        auto pCollect = one.second; // 获取系统集合
        const auto size = pCollect->GetAll()->size(); // 获取集合大小
        if (size <= 0)
            continue; // 如果集合为空，跳过

        total += size; // 累加实体总数

        // 输出集合的数量和类型
        std::stringstream log;
        log << " 数量:" << std::setw(5) << std::setfill(' ') << size << "    " << pCollect->GetClassType().c_str();
        LOG_DEBUG(log.str().c_str());

        // 检查集合是否是网络监听器类型
        if (strutil::stricmp(typeid(NetworkListen).name(), pCollect->GetClassType().c_str()) == 0)
        {
            pListen = dynamic_cast<NetworkListen*>(pCollect->Get()); // 转换为 NetworkListen 类型
        }
    }

    // 如果有网络监听器，显示其网络信息
    if (pListen != nullptr)
    {
        LOG_COLOR(LogColorGreen, "  网络类型:" << GetNetworkTypeName(pListen->GetNetworkType()));
        pListen->CmdShow();
    }

    LOG_DEBUG(" 实体总数:" << total);
}
