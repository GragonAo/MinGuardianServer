#include "efficiency_thread_component.h"
#include "message_system_help.h"
#include "thread_mgr.h"
#include "message_system.h"

// 效率线程组件实现
void EfficiencyThreadComponent::Awake(ThreadType threadType, std::thread::id threadId)
{
    _threadType = threadType; // 设置线程类型
    _efficiencyUpdateTime = 0; // 初始化效率更新时间

    std::stringstream ss;
    ss << threadId; // 将线程ID转换为字符串
    _threadId = ss.str(); // 存储线程ID

    // 添加定时器，定时调用 Sync 方法
    AddTimer(0, ThreadEfficiencyTime, false, 0, BindFunP0(this, &EfficiencyThreadComponent::Sync));
}

void EfficiencyThreadComponent::BackToPool()
{
    // 重置效率更新时间
    _efficiencyUpdateTime = 0;
}

void EfficiencyThreadComponent::UpdateTime(const uint64 disTime)
{
    // 更新效率时间，计算当前效率的平均值
    const auto puls = (_efficiencyUpdateTime + disTime) * 0.5f; // 计算平均值
    _efficiencyUpdateTime = puls; // 更新效率更新时间
}

void EfficiencyThreadComponent::Sync() const
{
    // 同步效率数据
    Proto::Efficiency protoTest;
    protoTest.set_time(_efficiencyUpdateTime); // 设置效率更新时间
    protoTest.set_thread_type((int)_threadType); // 设置线程类型
    protoTest.set_thread_id(_threadId); // 设置线程ID

    // 发送效率信息
    MessageSystemHelp::DispatchPacket(Proto::MI_Efficiency, protoTest, nullptr);
}
