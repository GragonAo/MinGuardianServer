#pragma once

#include "common.h"
#include "util_time.h"
#include "app_type.h"
#include "singleton.h"

#include <mutex>

// Global 类，管理全局应用状态和功能
class Global : public Singleton<Global>
{
public:
    // 构造函数，初始化应用类型和ID
    Global(APP_TYPE appType, int appId);
    
    // 更新时间
    void UpdateTime();

    // 根据序号获取应用ID
    static int GetAppIdFromSN(uint64 sn);

    // 生成唯一序号 (SN)
    uint64 GenerateSN(); // SN = 64位，包含时间戳 + 应用ID + ticket

    // 生成唯一标识符 (UUID)
    static std::string GenerateUUID();

    // 获取当前应用类型
    APP_TYPE GetCurAppType() const;

    // 获取当前应用ID
    int GetCurAppId() const;

    // 年天数
    int YearDay;

    // 时间戳
    timeutil::Time TimeTick;

    // 停止标志
    bool IsStop{ false };

private:
    // 互斥锁，用于线程安全
    std::mutex _mtx;

    // 序号票据，确保生成的SN唯一
    unsigned short _snTicket{ 1 };

    // 应用类型
    APP_TYPE _appType;

    // 应用ID
    int _appId{ 0 };
};
