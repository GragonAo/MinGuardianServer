#include "check_time_component.h"
#include "global.h"
#include "log4_help.h"
#include "component_help.h"
#include <thread>

// 开始计时，记录当前的时间戳
void CheckTimeComponent::CheckBegin()
{
    // 从全局对象获取当前时间戳，并保存到 _beginTick 变量中
    _beginTick = Global::GetInstance()->TimeTick;
}

// 记录指定检查点的执行时间
// 参数: 
//   key - 用于标识不同检查点的字符串
void CheckTimeComponent::CheckPoint(std::string key)
{
    // 如果当前检查点还没有记录过，初始化最大时间和平均时间
    if (_maxTicks.find(key) == _maxTicks.end())
    {
        _maxTicks[key] = 0;   // 初始化最大时间为 0
        _aveTime[key] = 0;    // 初始化平均时间为 0
    }

    // 计算从开始计时到当前检查点的时间差
    auto dis = Global::GetInstance()->TimeTick - _beginTick;

    // 更新平均时间，采用累加与平均的方式进行计算
    _aveTime[key] = (_aveTime[key] + dis) * 0.5f;

    // 如果当前检查点的时间差大于已记录的最大时间，则更新最大时间
    if (_maxTicks[key] < dis)
    {
        _maxTicks[key] = dis;

        // 如果定义了 LOG_TRACE_COMPONENT_OPEN 进行日志记录，并且平均时间超过 200ms
        #ifdef LOG_TRACE_COMPONENT_OPEN
        if (_aveTime[key] > 200)
        {
            // 记录日志，输出检查点的 key、最大时间、平均时间以及线程 ID
            std::stringstream os;
            os << "key:" << key.c_str() << " time:" << _maxTicks[key] 
               << " ave time:" << _aveTime[key];
            os << " thread id:" << std::this_thread::get_id(); // 记录当前线程 ID
            ComponentHelp::GetTraceComponent()->Trace(TraceType::Time, 0, os.str()); // 写入日志
        }
        #endif
    }
}
