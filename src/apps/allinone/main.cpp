#include "libresource/resource_manager.h"
#include "libserver/common.h"
#include "libserver/server_app.h"
#include "libserver/console.h"
#include "libserver/log4.h"
#include "libserver/component_help.h"

#include "login/login.h"
#include "dbmgr/dbmgr.h"
#include "appmgr/appmgr.h"
#include "game/game.h"
#include "space/space.h"
#include "libserver/network_listen.h"
#include "libserver/thread_type.h"
#include "libserver/global.h"
#include "libserver/network_connector.h"


int main(int argc, char *argv[])
{
    // 当前应用类型设置为 APP_ALLINONE，表示该应用集成了所有功能模块
    const APP_TYPE curAppType = APP_TYPE::APP_ALLINONE;

    // 创建一个服务器应用程序实例，传递当前应用类型和命令行参数
    ServerApp app(curAppType, argc, argv);
    // 初始化应用程序，加载配置、日志系统等
    app.Initialize();

    // 获取线程管理器的实例
    auto pThreadMgr = ThreadMgr::GetInstance();

    pThreadMgr->GetEntitySystem()->AddComponent<ResourceManager>();

    // 初始化应用管理模块，管理不同的应用组件
    InitializeComponentAppMgr(pThreadMgr);

    // 初始化数据库管理模块，处理数据库的相关操作
    InitializeComponentDBMgr(pThreadMgr);

    // 初始化登录模块，处理用户登录的相关逻辑
    InitializeComponentLogin(pThreadMgr);

    // 初始化游戏逻辑模块，处理游戏的核心逻辑
    InitializeComponentGame(pThreadMgr);

    // 初始化空间管理模块，处理游戏中的场景、地图等相关逻辑
    InitializeComponentSpace(pThreadMgr);

    // 创建一个网络监听组件，用于监听客户端的连接
    const auto pGlobal = Global::GetInstance();
    pThreadMgr->CreateComponent<NetworkListen>(ListenThread, false, (int)pGlobal->GetCurAppType(), pGlobal->GetCurAppId());

    // HTTP 监听：从 YAML 配置中获取应用程序的 IP 和 HTTP 端口，并创建网络监听器用于 HTTP 请求
    const auto pYaml = ComponentHelp::GetYaml();
    const auto pCommonConfig = pYaml->GetIPEndPoint(pGlobal->GetCurAppType(), pGlobal->GetCurAppId());
    pThreadMgr->CreateComponent<NetworkListen>(ListenThread, false, pCommonConfig->Ip, pCommonConfig->HttpPort);

    // 创建一个网络连接器组件，用于处理 HTTP 连接
    pThreadMgr->CreateComponent<NetworkConnector>(ConnectThread, false, (int)NetworkType::HttpConnector, 0);

    // 运行服务器应用程序，进入主循环，开始处理网络请求和游戏逻辑
    app.Run();
    // 释放资源，清理程序
    app.Dispose();

    return 0;
}
