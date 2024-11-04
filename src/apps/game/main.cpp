#include "libresource/resource_manager.h"
#include "libserver/server_app.h"
#include "game.h"
#include "libserver/global.h"
#include "libserver/network_listen.h"
#include "libserver/network_connector.h"

int main(int argc, char* argv[])
{
    // 设置当前应用程序的类型为游戏服务器
    const APP_TYPE curAppType = APP_TYPE::APP_GAME;

    // 创建一个服务器应用程序对象，并传入当前应用类型、命令行参数等信息
    ServerApp app(curAppType, argc, argv);

    // 初始化服务器应用程序
    app.Initialize();

    // 获取线程管理器的实例，用于管理应用中的各个组件
    auto pThreadMgr = ThreadMgr::GetInstance();

    pThreadMgr->GetEntitySystem()->AddComponent<ResourceManager>();

    // 初始化与游戏相关的组件
    InitializeComponentGame(pThreadMgr);

    // 获取全局变量的实例
    const auto pGlobal = Global::GetInstance();

    // 创建监听组件，用于监听客户端连接，绑定当前应用的类型和ID
    pThreadMgr->CreateComponent<NetworkListen>(ListenThread, false, (int)pGlobal->GetCurAppType(), pGlobal->GetCurAppId());

    // 创建网络连接器，用于与应用管理器、数据库管理器和空间服务器建立连接
    pThreadMgr->CreateComponent<NetworkConnector>(
        ConnectThread,              // 线程类型
        false,                      // 是否是单例
        (int)NetworkType::TcpConnector,  // 使用TCP连接
        (int)(APP_APPMGR | APP_DB_MGR | APP_SPACE)  // 连接的目标是应用管理器、数据库管理器和空间服务器
    );

    // 启动应用程序的主循环
    app.Run();

    // 程序结束前释放资源
    app.Dispose();

    return 0;
}
