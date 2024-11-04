#pragma once
#include "mysql_connector.h"  // 包含与 MySQL 连接相关的头文件
#include "libserver/thread_mgr.h"  // 包含线程管理器的头文件

// 定义一个内联函数用于初始化数据库管理器相关的组件
inline void InitializeComponentDBMgr(ThreadMgr* pThreadMgr)
{
    // 使用线程管理器创建一个 MysqlConnector 组件，并将其绑定到 MysqlThread 线程
    // 第一个参数是组件类型，第二个参数是线程标识符，第三个参数指定该组件是否在该线程中运行（true 表示运行在该线程中）
    pThreadMgr->CreateComponent<MysqlConnector>(MysqlThread, true);
}
