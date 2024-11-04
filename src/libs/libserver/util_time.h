#pragma once

#include "common.h"
#include "util_string.h"

#include <chrono>
#include <sys/timeb.h> // 该头文件用于获取系统时间

namespace timeutil
{
    typedef uint64 Time;  // 定义 Time 类型为毫秒，从1971年开始的时间戳

    // 添加秒数到给定的时间值
    inline Time AddSeconds(Time timeValue, int second)
    {
        return timeValue + (uint64)second * 1000; // 将秒转换为毫秒后加到时间值
    }

    // 添加毫秒数到给定的时间值
    inline Time AddMilliseconds(Time timeValue, int milliseconds)
    {
        return timeValue + milliseconds; // 直接加上毫秒数
    }

    // 将时间戳转换为字符串格式
    inline std::string ToString(uint64 timeTick)
    {
        const auto milli = timeTick % 1000; // 获取毫秒部分
        time_t tick = (time_t)(timeTick / 1000); // 转换为 time_t 类型，单位为秒
        struct tm* tm = localtime(&tick); // 将 time_t 转换为本地时间结构

        // 格式化字符串并返回
        return strutil::format("%d-%02d-%02d %02d:%02d:%02d.%03d",
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, milli);
    }
}
