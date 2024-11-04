#pragma once

#include "common.h"
#include "thread.h"
#include "cache_refresh.h"

class Packet;  // 前向声明，避免头文件循环依赖

// ThreadCollector 类：用于管理和操作多个线程
class ThreadCollector : public IDisposable
{
public:
    // 构造函数：根据线程类型和初始化线程数量创建 ThreadCollector 对象
    ThreadCollector(ThreadType threadType, int initNum);

    // 创建多个线程
    void CreateThread(int num);

    // 销毁所有线程
    void DestroyThread();

    // 更新所有线程的状态
    void Update();

    // 判断是否所有线程都已停止
    bool IsStopAll();

    // 判断是否所有线程都已销毁
    bool IsDestroyAll();

    // 重载自 IDisposable，清理资源
    void Dispose() override;

    // 处理消息的虚函数（可以被子类重写）
    virtual void HandlerMessage(Packet* pPacket);

    // 处理创建消息的虚函数（可以被子类重写）
    virtual void HandlerCreateMessage(Packet* pPacket, bool isToAllThread);

protected:
    ThreadType _threadType;  // 线程的类型，标识线程的用途
    CacheRefresh<Thread> _threads;  // 用于存储和管理线程的缓存刷新机制

    uint64 _nextThreadSn{ 0 };  // 下一个线程的序列号，用于标识新创建的线程
};
