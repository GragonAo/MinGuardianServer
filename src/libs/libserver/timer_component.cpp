#include "timer_component.h"
#include <utility>
#include "global.h"
#include "log4_help.h"
#include "update_component.h"

// 用于比较两个 Timer 对象的比较器结构体
struct CompareTimer
{
    // 重载操作符()，用于比较 Timer 的 NextTime，返回较大的时间值
    constexpr bool operator()(const Timer& _Left, const Timer& _Right) const
    {
        return (_Left.NextTime > _Right.NextTime);
    }
};

// 添加 Timer 到时间堆中
void TimerComponent::Add(Timer& data)
{
    //LOG_DEBUG("add time call." << data.Handler.target_type().name());

    // 将计时器对象加入时间堆中
    _heap.emplace_back(data);
    
    // 如果堆中只有一个元素，创建堆
    if (_heap.size() == 1)
    {
        make_heap(_heap.begin(), _heap.end(), CompareTimer());
    }
    else
    {
        // 如果已有元素，则使用 push_heap 保持堆的结构
        push_heap(_heap.begin(), _heap.end(), CompareTimer());
    }
}

// 初始化 Timer 组件，设置 Update 回调函数
void TimerComponent::Awake()
{
    // 添加 UpdateComponent，并将其 Update 函数绑定为 TimerComponent 的 Update 函数
    AddComponent<UpdateComponent>(BindFunP0(this, &TimerComponent::Update));
}

// 重置计时器组件，将时间堆清空
void TimerComponent::BackToPool()
{
    _heap.clear();
}

// 添加新的计时器
uint64 TimerComponent::Add(const int total, const int durations, const bool immediateDo, const int immediateDoDelaySecond, TimerHandleFunction handler)
{
    // durations: 执行周期
    // immediateDo: 是否立即执行
    // immediateDoDelaySecond: 初次执行的延迟时间（秒）

    Timer data;
    data.SN = Global::GetInstance()->GenerateSN();  // 生成唯一序列号
    data.CallCountCur = 0;  // 当前执行次数初始化为 0
    data.CallCountTotal = total;  // 总执行次数
    data.DurationSecond = durations;  // 设置执行周期
    data.Handler = std::move(handler);  // 设置触发时执行的回调函数
    data.NextTime = timeutil::AddSeconds(Global::GetInstance()->TimeTick, durations);  // 设置下一次执行的时间

    // 如果立即执行，将下一次执行时间设置为延迟后的时间
    if (immediateDo)
    {
        data.NextTime = timeutil::AddSeconds(Global::GetInstance()->TimeTick, immediateDoDelaySecond);
    }

    Add(data);  // 将计时器加入堆中
    return data.SN;  // 返回计时器的唯一序列号
}

// 移除指定序列号的计时器
void TimerComponent::Remove(std::list<uint64>& timers)
{
    // 遍历要删除的计时器序列号
    for (auto sn : timers)
    {
        // 在堆中找到匹配的计时器
        auto iter = std::find_if(_heap.begin(), _heap.end(), [sn](const Timer& one)
            {
                return one.SN == sn;  // 根据序列号匹配
            });

        // 如果没找到则直接返回
        if (iter == _heap.end())
            return;

        // 从堆中删除该计时器
        _heap.erase(iter);
    }

    // 删除后重新构建堆
    make_heap(_heap.begin(), _heap.end(), CompareTimer());
}

// 检查是否有计时器时间到达
bool TimerComponent::CheckTime()
{
    if (_heap.empty())
        return false;

    // 检查堆顶计时器是否已经达到触发时间
    const auto data = _heap.front();
    return data.NextTime <= Global::GetInstance()->TimeTick;
}

// 弹出堆顶的计时器
Timer TimerComponent::PopTimeHeap()
{
    // 弹出堆顶元素
    pop_heap(_heap.begin(), _heap.end(), CompareTimer());

    // 获取堆顶计时器，并将其从堆中删除
    Timer data = _heap.back();
    _heap.pop_back();

    return data;
}

// 更新函数，定期检查并处理所有计时器
void TimerComponent::Update()
{
    // 循环检查所有到达时间的计时器
    while (CheckTime())
    {
        // 弹出堆顶计时器
        Timer data = PopTimeHeap();

        // 执行计时器的回调函数
        data.Handler();

        // 如果不是无限次执行，将当前执行次数加一
        if (data.CallCountTotal != 0)
            data.CallCountCur++;

        // 如果达到总执行次数，计时器任务完成
        if (data.CallCountTotal != 0 && data.CallCountCur >= data.CallCountTotal)
        {
            // 计时器任务完成后自动删除
        }
        else
        {
            // 重新设置下次触发时间，并将计时器加入堆中
            data.NextTime = timeutil::AddSeconds(Global::GetInstance()->TimeTick, data.DurationSecond);
            Add(data);
        }
    }
}
