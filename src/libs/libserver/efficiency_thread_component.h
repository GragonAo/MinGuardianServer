#pragma once
#include "system.h"
#include "thread_type.h"
#include "entity.h"

#include <thread>

#define ThreadEfficiencyTime 5 // 定义线程效率更新的时间间隔（毫秒）

class Packet;

class EfficiencyThreadComponent : public Entity<EfficiencyThreadComponent>, public IAwakeSystem<ThreadType, std::thread::id>
{
public:
    // 初始化方法，接收线程类型和线程ID
    void Awake(ThreadType threadType, std::thread::id threadId) override;

    // 释放资源，返回对象到池中
    void BackToPool() override;

    // 更新效率时间
    void UpdateTime(uint64 disTime);

    // 同步效率数据
    void Sync() const;

private:
    // 用于测试更新效率的时间
    uint64 _efficiencyUpdateTime{ 0 }; // 初始化为0

    std::string _threadId{ "" }; // 存储线程ID
    ThreadType _threadType{ ThreadType::MainThread }; // 初始化线程类型为主线程
};
