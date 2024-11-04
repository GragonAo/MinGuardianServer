#include "object_pool_packet.h"
#include "component_help.h"
#include "socket_object.h"

/**
 * @brief 从对象池中分配一个新的 Packet 对象，并初始化消息 ID 和网络标识。
 * 
 * @param msgId 消息 ID，用于标识消息类型。
 * @param pIdentify 指向 NetworkIdentify 的指针，用于标识网络连接的唯一标识。
 * @return Packet* 返回分配的 Packet 对象指针。
 */
Packet* DynamicPacketPool::MallocPacket(Proto::MsgId msgId, NetIdentify* pIdentify)
{
    // 使用互斥锁保护多线程环境下的对象分配操作
    std::lock_guard<std::mutex> guard(_packet_lock);

    // 调用对象池中的 MallocObject 方法分配 Packet 对象
    const auto pPacket = DynamicObjectPool<Packet>::MallocObject(nullptr,nullptr,0, msgId, pIdentify);

#ifdef LOG_TRACE_COMPONENT_OPEN
    // 如果启用了日志跟踪，记录分配的 Packet 信息
    if (pIdentify != nullptr)
    {
        const google::protobuf::EnumDescriptor* descriptor = Proto::MsgId_descriptor();
        const auto name = descriptor->FindValueByNumber(msgId)->name();
        std::stringstream os;
        os << "malloc.  "
            << " sn:" << pPacket->GetSN()                // Packet 序列号
            << " msgId:" << name.c_str()                 // 消息 ID 的名称
            << " " << dynamic_cast<NetIdentify*>(pPacket); // 打印 Packet 对应的网络标识
        ComponentHelp::GetTraceComponent()->Trace(TraceType::Packet, pIdentify->GetSocketKey()->Socket, os.str());
    }
#endif

    return pPacket;
}

#ifdef LOG_TRACE_COMPONENT_OPEN
/**
 * @brief 跟踪 Packet 对象的释放，记录相关的日志信息。
 * 
 * @param pPacket 要释放的 Packet 对象。
 */
inline void TraceFree(Packet* pPacket)
{
    const google::protobuf::EnumDescriptor* descriptor = Proto::MsgId_descriptor();
    const auto name = descriptor->FindValueByNumber(pPacket->GetMsgId())->name();
    std::stringstream os;
    os << "free.    "
        << " sn:" << pPacket->GetSN()                // Packet 序列号
        << " msgId:" << name.c_str()                 // 消息 ID 的名称
        << " " << dynamic_cast<NetIdentify*>(pPacket); // 打印 Packet 对应的网络标识
    ComponentHelp::GetTraceComponent()->Trace(TraceType::Packet, pPacket->GetSocketKey()->Socket, os.str());
}
#endif

/**
 * @brief 定期更新对象池的状态，检查是否有 Packet 对象需要释放回对象池。
 */
void DynamicPacketPool::Update()
{
    // 使用互斥锁保护对象池更新操作
    std::lock_guard<std::mutex> guard(_packet_lock);

    // 调用基类的 Update 方法进行常规更新
    DynamicObjectPool<Packet>::Update();

    // 获取对象池中正在使用的对象列表
    auto lists = _objInUse.GetReaderCache();
    for (auto iter = lists->begin(); iter != lists->end(); ++iter)
    {
        auto pPacket = iter->second;
        // 检查 Packet 对象是否可以释放回池中
        if (pPacket->CanBack2Pool())
        {
#ifdef LOG_TRACE_COMPONENT_OPEN
            // 如果启用了日志跟踪，记录释放的 Packet 信息
            TraceFree(pPacket);
#endif
            // 释放对象到池中
            DynamicObjectPool<Packet>::FreeObject(pPacket);
        }
    }
}

/**
 * @brief 将 Packet 对象释放回对象池中，并记录释放的日志信息（如果启用）。
 * 
 * @param pObj 要释放的对象，一般是 Packet 对象。
 */
void DynamicPacketPool::FreeObject(IComponent* pObj)
{
    // 使用互斥锁保护对象释放操作
    std::lock_guard<std::mutex> guard(_packet_lock);

#ifdef LOG_TRACE_COMPONENT_OPEN
    // 如果启用了日志跟踪，记录释放的 Packet 信息
    TraceFree(dynamic_cast<Packet*>(pObj));
#endif

    // 调用基类的 FreeObject 方法释放对象
    DynamicObjectPool<Packet>::FreeObject(pObj);
}

/**
 * @brief 显示对象池当前的状态，输出各个消息 ID 的统计信息。
 */
void DynamicPacketPool::Show()
{
    // 使用互斥锁保护对象池状态显示操作
    std::lock_guard<std::mutex> guard(_packet_lock);

    // 创建一个统计数据的映射，统计各个消息 ID 的使用情况
    std::map<int, int> statData;
    auto lists = _objInUse.GetReaderCache();
    for (auto iter = lists->begin(); iter != lists->end(); ++iter)
    {
        auto pPacket = iter->second;
        int msgId = pPacket->GetMsgId();
        auto iterMsgId = statData.find(msgId);
        if (iterMsgId == statData.end()) {
            statData.insert(std::make_pair(msgId, 0));
        }
        statData[msgId]++;
    }

    // 获取消息 ID 的描述符
    const google::protobuf::EnumDescriptor* descriptor = Proto::MsgId_descriptor();
    for (auto iter = statData.begin(); iter != statData.end(); ++iter)
    {
        auto name = descriptor->FindValueByNumber(iter->first)->name();
        LOG_WARN(" msgId:" << name.c_str() << " " << iter->second); // 输出每个消息 ID 的名称和计数
    }

    // 显示对象池中的其他信息，调用基类的 Show 方法
    DynamicObjectPool<Packet>::Show();
}
