#include "thread_collector_exclusive.h"
#include "packet.h"
#include "message_system.h"

// 构造函数，初始化ThreadCollector对象，传入线程类型和初始线程数
ThreadCollectorExclusive::ThreadCollectorExclusive(ThreadType threadType, int initNum) 
    : ThreadCollector(threadType, initNum) 
{
}

// 处理消息函数，根据消息ID执行不同的操作
void ThreadCollectorExclusive::HandlerMessage(Packet* pPacket)
{
    // 如果消息ID为MI_CmdThread，表示命令线程，每个线程都会执行
    if (pPacket->GetMsgId() == Proto::MsgId::MI_CmdThread)
    {
        // 调用HandlerCreateMessage处理消息，并指定为每个线程都执行
        HandlerCreateMessage(pPacket, true);
    }
    else
    {
        // 获取线程的缓存数据
        auto objs = _threads.GetReaderCache();
        // 查找下一个线程的SN
        auto iter = objs->find(_nextThreadSn);
        if (iter == objs->end()) 
        {
            // 如果没有找到，重置为开始位置
            iter = objs->begin();
        }

        // 将数据包添加到线程的消息系统中
        iter->second->GetMessageSystem()->AddPacketToList(pPacket);
        iter++;

        // 循环分配下一个线程
        if (iter == objs->end()) 
        {
            iter = objs->begin();
        }

        // 更新下一个线程的SN
        _nextThreadSn = iter->first;
    }
}
