#pragma once
#include "common.h"

// 定义应用类型的枚举，使用位运算表示不同的应用类型
enum APP_TYPE
{
    // APP_Global 和 APP_None 值相同，APP_Global 仅用于全局读取
    APP_None = 0,            // 无类型
    APP_Global = 0,          // 全局类型

    APP_DB_MGR = 1,          // 数据库管理应用
    APP_GAME_MGR = 1 << 1,   // 游戏管理应用
    APP_SPACE_MGR = 1 << 2,  // 空间管理应用

    APP_LOGIN = 1 << 3,      // 登录服务应用
    APP_GAME = 1 << 4,       // 游戏应用
    APP_SPACE = 1 << 5,      // 空间应用
    APP_ROBOT = 1 << 6,      // 机器人应用

    // APP_APPMGR 是 APP_GAME_MGR 和 APP_SPACE_MGR 的组合
    APP_APPMGR = APP_GAME_MGR | APP_SPACE_MGR,

    // APP_ALLINONE 包含所有应用类型的组合
    APP_ALLINONE = APP_DB_MGR | APP_GAME_MGR | APP_SPACE_MGR | APP_LOGIN | APP_GAME | APP_SPACE,
};

// 通过 appType 和 appId 生成应用的唯一键（appKey）
// appKey 通过将 appType 左移 32 位并加上 appId 得到
inline const uint64 GetAppKey(int appType, int appId)
{
    // 将 appType 左移 32 位，并加上 appId，生成唯一的 appKey
    return ((uint64)appType << 32) + appId;
}

// 重载 GetAppKey 函数，直接使用 APP_TYPE 枚举类型作为参数
inline const uint64 GetAppKey(APP_TYPE appType, int appId)
{
    // 调用上一个 GetAppKey 函数，传入 appType 的整数形式
    return GetAppKey((int)appType, appId);
}

// 从 appKey 中获取应用的类型
inline const APP_TYPE GetTypeFromAppKey(uint64 appKey)
{
    // 将 appKey 右移 32 位，提取出应用类型
    return (APP_TYPE)(appKey >> 32);
}

// 从 appKey 中获取应用的 ID
inline const int GetIdFromAppKey(uint64 appKey)
{
    // 使用位掩码 0x0000FFFF 获取 appKey 的低 16 位，即应用 ID
    return (int)(appKey & 0x0000FFFF);
}

// 根据 APP_TYPE 返回对应的应用名称字符串
inline const char* GetAppName(const APP_TYPE appType)
{
    // 如果应用类型是 APP_ALLINONE，返回 "allinone"
    if (appType == APP_TYPE::APP_ALLINONE)
        return "allinone";

    // 如果应用类型是 APP_ROBOT，返回 "robot"
    if (appType == APP_TYPE::APP_ROBOT)
        return "robot";

    // 如果应用类型是 APP_SPACE，返回 "space"
    if (appType == APP_TYPE::APP_SPACE)
        return "space";

    // 如果应用类型是 APP_GAME，返回 "game"
    if (appType == APP_TYPE::APP_GAME)
        return "game";

    // 如果应用类型是 APP_LOGIN，返回 "login"
    if (appType == APP_TYPE::APP_LOGIN)
        return "login";

    // 如果应用类型是 APP_DB_MGR，返回 "dbmgr"
    if (appType == APP_TYPE::APP_DB_MGR)
        return "dbmgr";

    // 如果应用类型是 APP_APPMGR，返回 "appmgr"
    if (appType == APP_TYPE::APP_APPMGR)
        return "appmgr";

    // 如果应用类型是 APP_None，返回 "none"
    if (appType == APP_TYPE::APP_None)
        return "none";

    // 如果没有匹配的类型，返回 "unknown"
    return "unknown";
}
