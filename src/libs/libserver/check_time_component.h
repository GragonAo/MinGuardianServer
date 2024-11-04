#pragma once
#include "common.h"

// CheckTimeComponent 类用于记录和检查某些操作的执行时间
class CheckTimeComponent
{
public:
    // 开始计时，记录起始时间戳
    void CheckBegin();

    // 检查某个特定时间点的时间，保存对应的时间间隔
    // 参数:
    //   key - 标识检查点的唯一字符串，用于存储和记录此检查点的时间
    void CheckPoint(std::string key);

protected:
    uint64 _beginTick;  // 开始计时的时间戳，记录计时开始的瞬间

    // 用于存储每个检查点的平均执行时间
    // key 为检查点的名称，value 为该检查点累计的平均执行时间
    std::map<std::string, uint64> _aveTime;

    // 用于记录每个检查点的最大耗时（即执行时间的最大值）
    // key 为检查点的名称，value 为该检查点执行过程中记录的最大时间间隔
    std::map<std::string, uint64> _maxTicks;
};
