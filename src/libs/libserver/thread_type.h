
#pragma once

enum ThreadType
{
    MainThread = 1 << 0,       // 主线程
    ListenThread = 1 << 1,     // 监听线程
    ConnectThread = 1 << 2,    // 连接线程
    LogicThread = 1 << 3,      // 逻辑线程
    MysqlThread = 1 << 4,      // 数据库线程
    AllThreadType = MainThread | LogicThread | ListenThread | ConnectThread | MysqlThread
};

inline const char* GetThreadTypeName(const ThreadType threadType)
{
    if (threadType == MainThread)
        return "MainThread";

    if (threadType == LogicThread)
        return "LogicThread";

    if (threadType == ListenThread)
        return "ListenThread";

    if (threadType == ConnectThread)
        return "ConnectThread";

    if (threadType == MysqlThread)
        return "MysqlThread";

    return "unknown";
}
