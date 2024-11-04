#pragma once

#include "libserver/thread_mgr.h"
#include "account.h"
#include "redis_login.h"

// 该函数用于在线程管理器中初始化登录相关的组件
inline void InitializeComponentLogin(ThreadMgr* pThreadMgr)
{
    // 使用线程管理器创建并注册一个Account组件
    // Account组件用于处理与账户相关的功能，比如登录、注册等
    pThreadMgr->CreateComponent<Account>();
    //RedisLogin组件用于处理账户Redis相关的功能，如账户是否在线、生成Token等
    pThreadMgr->CreateComponent<RedisLogin>();
}
