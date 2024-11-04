#pragma once
#include "thread_collector.h"

// 专用线程收集器类，继承自 ThreadCollector
class ThreadCollectorExclusive : public ThreadCollector
{
public:
    // 构造函数：接收线程类型和初始线程数量作为参数
    ThreadCollectorExclusive(ThreadType threadType, int initNum);

    // 重写的消息处理函数：用于处理传入的 Packet 消息
    virtual void HandlerMessage(Packet* pPacket) override;
};
