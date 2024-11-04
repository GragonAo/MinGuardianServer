#pragma once

#include "system.h"
#include "entity.h"
#include "thread_type.h"
#include <mutex>

class Packet;

// ConsoleThreadComponent 负责处理与线程相关的控制台命令
class ConsoleThreadComponent : public Entity<ConsoleThreadComponent>, public IAwakeSystem<ThreadType>
{
public:
    // 初始化方法，接受线程类型参数
    void Awake(ThreadType iType) override;

    // 资源回收方法
    void BackToPool() override;

private:
    // 处理线程命令的各种方法
    void HandleCmdThread(Packet* pPacket);
    void HandleCmdThreadEntities(Packet* pPacket);
    void HandleCmdThreadPool(Packet* pPacket);
    void HandleCmdThreadConnect(Packet* pPacket);

private:
    ThreadType _threadType;  // 当前线程的类型

    static std::mutex _show_lock;  // 用于多线程环境下的输出锁
};
