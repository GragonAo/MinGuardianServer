#pragma once
#include "system.h"
#include "entity.h"
#include "util_time.h"

#include <map>

class Packet;

// 线程效率信息结构体
struct ThreadEfficiencyInfo
{
    ThreadType ThreadTypeKey;     // 线程类型键
    uint64 UpdateTime;            // 更新平均时间
    timeutil::Time LastRecvTime;  // 上次接收时间
    uint64 UpdateTimeMax;         // 最大更新时间
};

// 控制台效率组件类
class ConsoleEfficiencyComponent : public Entity<ConsoleEfficiencyComponent>, public IAwakeSystem<>
{
public:
    // 初始化
    void Awake() override;
    
    // 归还对象到池
    void BackToPool() override;

private:
    // 处理效率命令
    void HandleCmdEfficiency(Packet* pPacket);
    
    // 处理效率数据
    void HandleEfficiency(Packet* pPacket);

private:
    // 线程ID与线程效率信息的映射
    std::map<std::string, ThreadEfficiencyInfo> _threads;
};
