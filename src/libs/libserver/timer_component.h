#pragma once
#include "util_time.h"
#include "system.h"
#include "entity.h"

#include <list>
#include <vector>

// 定义一个结构体来表示一个计时器
struct Timer
{
    // 下一次触发时间，使用时间工具类中的 `Time` 表示
    timeutil::Time NextTime;

    // 计时器触发时执行的函数句柄
    TimerHandleFunction Handler;

    // 初次执行的延迟时间（秒）
    int DelaySecond;

    // 执行周期（秒），表示每隔多少秒触发一次
    int DurationSecond;

    // 总共触发的次数，0 表示无限次
    int CallCountTotal;

    // 当前已触发的次数
    int CallCountCur;

    // 用于标识 Timer 的唯一序列号
    uint64 SN;
};

// TimerComponent 类继承自 Entity 和 IAwakeSystem，用于管理多个计时器
class TimerComponent : public Entity<TimerComponent>, public IAwakeSystem<>
{
public:
    // 初始化函数
    void Awake() override;
    
    // 重置函数，将组件重置到对象池中
    void BackToPool() override;

    // 添加一个计时器
    // 参数:
    // - total: 总执行次数（0 为无限）
    // - durations: 执行周期（秒）
    // - immediateDo: 是否立即执行
    // - immediateDoDelaySecond: 延迟立即执行的时间
    // - handler: 计时器触发时执行的回调函数
    // 返回值:
    // - 返回计时器的唯一序列号
    uint64 Add(int total, int durations, bool immediateDo, int immediateDoDelaySecond, TimerHandleFunction handler);

    // 移除计时器，通过传入计时器序列号列表
    void Remove(std::list<uint64>& timers);

    // 检查当前时间是否有计时器触发
    bool CheckTime();

    // 从时间堆中弹出一个即将触发的计时器
    Timer PopTimeHeap();

    // 更新函数，用于定期检查并处理所有计时器
    void Update();

protected:
    // 内部函数，将一个计时器加入到时间堆中
    void Add(Timer& data);

private:
    // 使用 vector 作为计时器堆，用于存储并按时间排序
    std::vector<Timer> _heap;
};
