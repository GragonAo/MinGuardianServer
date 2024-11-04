#include "libserver/common.h"          // 包含通用的服务器头文件
#include "libserver/server_app.h"       // 包含服务器应用程序的定义
#include "libserver/global.h"           // 包含全局变量和设置的定义
#include "libserver/network_listen.h"   // 包含网络监听器的定义

#include "space.h"                      // 包含空间管理相关的定义
#include "libresource/resource_manager.h" // 包含资源管理器的定义
#include "libserver/network_connector.h" // 包含网络连接器的定义

int main(int argc, char* argv[])
{
    // 设置当前应用程序类型为空间服务器（APP_SPACE）
    const APP_TYPE curAppType = APP_TYPE::APP_SPACE;
    
    // 创建服务器应用程序实例，并传入应用类型和命令行参数
    ServerApp app(curAppType, argc, argv);
    app.Initialize(); // 初始化服务器应用程序

    // 获取线程管理器的实例
    auto pThreadMgr = ThreadMgr::GetInstance();

    // 添加资源管理器组件，用于管理资源
    pThreadMgr->GetEntitySystem()->AddComponent<ResourceManager>();

    // 初始化空间组件，用于设置空间逻辑
    InitializeComponentSpace(pThreadMgr);

    // 获取全局变量的实例
    const auto pGlobal = Global::GetInstance();
    
    // 创建网络监听器组件，监听客户端的连接请求
    pThreadMgr->CreateComponent<NetworkListen>(
        ListenThread,           // 监听线程
        false,                  // 是否为多线程组件
        (int)pGlobal->GetCurAppType(),  // 当前应用程序类型
        (int)pGlobal->GetCurAppId()     // 当前应用程序ID
    );

    // 创建网络连接器组件，用于连接到其他应用程序管理器和数据库管理器
    pThreadMgr->CreateComponent<NetworkConnector>(
        ConnectThread,          // 连接线程
        false,                  // 是否为多线程组件
        (int)NetworkType::TcpConnector, // 网络类型为TCP连接器
        (int)(APP_APPMGR | APP_DB_MGR)  // 目标应用程序为应用程序管理器和数据库管理器
    );

    // 运行服务器应用程序的主循环
    app.Run();
    
    // 释放资源并清理
    app.Dispose();

    return 0; // 结束程序
}
