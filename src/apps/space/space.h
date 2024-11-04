#pragma once
#include "libserver/thread_mgr.h"            // 引入线程管理器头文件

#include "console_cmd_world.h"                // 引入控制台命令世界的头文件
#include "world_operator_component.h"         // 引入世界操作组件的头文件
#include "world_gather.h"                     // 引入世界聚集的头文件
#include "move_system.h"                      // 引入移动系统的头文件

// 初始化组件空间的函数
inline void InitializeComponentSpace(ThreadMgr* pThreadMgr)
{
    // 创建世界聚集组件
    pThreadMgr->CreateComponent<WorldGather>();

    // 创建世界操作组件
    pThreadMgr->CreateComponent<WorldOperatorComponent>();

    // 获取控制台组件并注册控制台命令
    auto pConsole = pThreadMgr->GetEntitySystem()->GetComponent<Console>();
    pConsole->Register<ConsoleCmdWorld>("world");

    // 创建移动系统
    pThreadMgr->CreateSystem<MoveSystem>();
}
