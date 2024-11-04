#include "libresource/resource_manager.h"
#include "libserver/common.h"
#include "libserver/server_app.h"
#include "libserver/network_listen.h"
#include "libserver/network_connector.h"

#include "login.h"
#include "libserver/global.h"

int main(int argc, char* argv[])
{
    // 定义当前应用类型为登录服务器
    const APP_TYPE curAppType = APP_TYPE::APP_LOGIN;

    // 创建ServerApp实例，传入应用类型、命令行参数用于初始化服务器应用
    ServerApp app(curAppType, argc, argv);
    app.Initialize();  // 初始化应用程序

    // 获取全局对象实例，用于访问应用级别的配置信息
    auto pGlobal = Global::GetInstance();

    // 获取线程管理器实例，管理多线程中的组件
    auto pThreadMgr = ThreadMgr::GetInstance();

    pThreadMgr->GetEntitySystem()->AddComponent<ResourceManager>();

    // 初始化登录模块的相关组件
    InitializeComponentLogin(pThreadMgr);

    // 创建网络监听组件，用于监听客户端的连接请求
    // ListenThread 是线程模式参数，false 表示不启用某些特性
    // GetCurAppType 返回当前应用的类型，用于网络监听的标识
    pThreadMgr->CreateComponent<NetworkListen>(ListenThread, false, (int)pGlobal->GetCurAppType(), pGlobal->GetCurAppId());

    // 创建两个网络连接器组件，用于连接到其它服务
    // 第一个用于 TCP 连接，连接到 App Manager 和 DB Manager
    pThreadMgr->CreateComponent<NetworkConnector>(ConnectThread, false, (int)NetworkType::TcpConnector, (int)(APP_APPMGR | APP_DB_MGR));

    // 第二个网络连接器用于 HTTP 连接，这里暂时没有指定连接的对象
    pThreadMgr->CreateComponent<NetworkConnector>(ConnectThread, false, (int)NetworkType::HttpConnector, 0);

    // 启动服务器应用，进入主循环，处理各种消息
    app.Run();

    // 释放服务器应用的资源，清理内存和关闭组件
    app.Dispose();

    return 0;
}
