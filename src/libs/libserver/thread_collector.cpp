#include "thread_collector.h"
#include "message_system.h"
#include "packet.h"

// 构造函数：根据线程类型和初始化线程数量创建 ThreadCollector 对象
ThreadCollector::ThreadCollector(const ThreadType threadType, int initNum)
{
    _threadType = threadType;
    _nextThreadSn = 0;  // 初始化下一个线程的序列号

    CreateThread(initNum);  // 创建指定数量的线程
}

// 创建指定数量的线程
void ThreadCollector::CreateThread(int num)
{
    for (int i = 0; i < num; i++)
    {
        const auto pThread = new Thread(_threadType);  // 根据线程类型创建新线程
        _threads.AddObj(pThread);  // 将线程添加到线程管理容器中
    }

    _threads.Swap(nullptr);  // 切换到新的线程缓存
    auto pList = _threads.GetReaderCache();  // 获取当前可读取的线程缓存
    for (auto iter = pList->begin(); iter != pList->end(); ++iter)
    {
        iter->second->Start();  // 启动每个线程
    }
}

// 销毁所有线程
void ThreadCollector::DestroyThread()
{
    auto pList = _threads.GetReaderCache();  // 获取当前可读取的线程缓存
    for (auto iter = pList->begin(); iter != pList->end(); ++iter)
    {
        iter->second->DestroyThread();  // 调用每个线程的销毁函数
    }
}

// 更新线程状态
void ThreadCollector::Update()
{
    if (_threads.CanSwap())  // 如果缓存可以交换
    {
        _threads.Swap(nullptr);  // 切换到新的线程缓存
    }
}

// 检查是否所有线程都已停止
bool ThreadCollector::IsStopAll()
{
    auto pList = _threads.GetReaderCache();  // 获取当前可读取的线程缓存
    for (auto iter = pList->begin(); iter != pList->end(); ++iter)
    {
        if (!iter->second->IsStop())  // 如果有任何线程没有停止，返回 false
            return false;
    }

    return true;  // 所有线程都已停止，返回 true
}

// 检查是否所有线程都已销毁
bool ThreadCollector::IsDestroyAll()
{
    auto pList = _threads.GetReaderCache();  // 获取当前可读取的线程缓存
    for (auto iter = pList->begin(); iter != pList->end(); ++iter)
    {
        if (!iter->second->IsDestroy())  // 如果有任何线程没有销毁，返回 false
            return false;
    }

    return true;  // 所有线程都已销毁，返回 true
}

// 资源释放函数，调用时清理所有资源
void ThreadCollector::Dispose()
{
    _threads.Dispose();  // 清理线程池中的资源
}

// 处理消息包，将消息分发给所有线程的消息系统
void ThreadCollector::HandlerMessage(Packet* pPacket)
{
    auto pList = _threads.GetReaderCache();  // 获取当前可读取的线程缓存
    for (auto iter = pList->begin(); iter != pList->end(); ++iter)
    {
        iter->second->GetMessageSystem()->AddPacketToList(pPacket);  // 将消息添加到每个线程的消息系统中
    }
}

// 处理创建消息，决定是发送给所有线程还是特定线程
void ThreadCollector::HandlerCreateMessage(Packet* pPacket, bool isToAllThread)
{
    if (isToAllThread)  // 如果消息需要发送到所有线程
    {
        auto pList = _threads.GetReaderCache();  // 获取线程列表
        for (auto iter = pList->begin(); iter != pList->end(); ++iter)
        {
            iter->second->GetMessageSystem()->AddPacketToList(pPacket);  // 分发消息到所有线程
        }
    }
    else  // 如果消息只发给某个特定线程
    {
        auto objs = _threads.GetReaderCache();  // 获取线程列表
        auto iter = objs->find(_nextThreadSn);  // 找到下一个要处理消息的线程
        if (iter == objs->end())  // 如果没有找到，选择第一个线程
        {
            iter = objs->begin();
        }

        iter->second->GetMessageSystem()->AddPacketToList(pPacket);  // 将消息添加到特定线程的消息系统
        ++iter;  // 移动到下一个线程
        if (iter == objs->end())  // 如果到达末尾，循环回第一个线程
        {
            iter = objs->begin();
        }

        _nextThreadSn = iter->first;  // 更新下一个要处理消息的线程序号
    }
}
