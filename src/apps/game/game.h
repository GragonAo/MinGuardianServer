#pragma once
#include "libserver/thread_mgr.h"
#include "world_proxy_gather.h"
#include "lobby.h"
#include "console_cmd_world_proxy.h"
#include "redis_game.h"
#include "world_proxy_locator.h"
#include "space_sync_handler.h"
#include "libserver/socket_locator.h"

inline void InitializeComponentGame(ThreadMgr* pThreadMgr)
{
    // 创建并初始化游戏组件
    pThreadMgr->CreateComponent<Lobby>();
    pThreadMgr->CreateComponent<WorldProxyGather>();
    pThreadMgr->CreateComponent<RedisGame>();

    // 添加全局组件到实体系统
    pThreadMgr->GetEntitySystem()->AddComponent<WorldProxyLocator>();
    pThreadMgr->GetEntitySystem()->AddComponent<SpaceSyncHandler>();
    pThreadMgr->GetEntitySystem()->AddComponent<SocketLocator>();

    // 获取控制台组件并注册控制台命令
    auto pConsole = pThreadMgr->GetEntitySystem()->GetComponent<Console>();
    pConsole->Register<ConsoleCmdWorldProxy>("proxy");
}
