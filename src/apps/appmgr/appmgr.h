#pragma once

#include "create_world_component.h"
#include "libserver/console.h"  // 引入控制台命令相关的库
#include "app_sync_component.h"  // 引入应用同步组件的定义
#include "console_cmd_create.h"     // 引入控制台命令的定义
#include "libserver/thread_mgr.h"

// 初始化应用管理器组件的函数
inline void InitializeComponentAppMgr(ThreadMgr* pThreadMgr)
{
    // 在线程管理器中创建应用同步组件
	pThreadMgr->CreateComponent<AppSyncComponent>();

    pThreadMgr->CreateComponent<CreateWorldComponent>();

    // 获取控制台组件
    auto pConsole = pThreadMgr->GetEntitySystem()->GetComponent<Console>();
    
    // 注册控制台命令“app”，该命令将与 ConsoleCmdApp 相关联
    pConsole->Register<ConsoleCmdCreate>("create");
}
