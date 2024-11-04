#include "libserver/server_app.h"            // 包含服务器应用程序类定义
#include "libserver/network_listen.h"        // 包含网络监听组件类定义
#include "libserver/component_help.h"        // 包含组件助手类，用于获取配置信息
#include "libserver/app_type.h"              // 定义应用程序类型枚举
#include "libserver/global.h"                // 包含全局对象，用于获取应用程序信息

#include "appmgr.h"                          // 包含应用管理器的具体实现头文件

int main(int argc, char *argv[])
{
    // 定义当前应用程序类型为 APP_APPMGR，表示应用管理器
    const APP_TYPE curAppType = APP_TYPE::APP_APPMGR;

    // 创建 ServerApp 实例，传入应用类型、命令行参数进行初始化
    ServerApp app(curAppType, argc, argv);
    app.Initialize();

    // 获取线程管理器实例
    auto pThreadMgr = ThreadMgr::GetInstance();

    // 调用函数初始化应用管理器相关组件
    InitializeComponentAppMgr(pThreadMgr);

    // 获取全局对象实例，获取应用的当前类型和ID
    const auto pGlobal = Global::GetInstance();

    // 创建网络监听组件，监听普通应用类型的连接请求
    pThreadMgr->CreateComponent<NetworkListen>(ListenThread, false, (int)pGlobal->GetCurAppType(), pGlobal->GetCurAppId());

    // 从 YAML 配置文件中获取应用程序的 IP 和端口
    const auto pYaml = ComponentHelp::GetYaml();
    const auto pCommonConfig = pYaml->GetIPEndPoint(pGlobal->GetCurAppType(), pGlobal->GetCurAppId());

    // 创建 HTTP 监听组件，监听 HTTP 请求
    pThreadMgr->CreateComponent<NetworkListen>(ListenThread, false, pCommonConfig->Ip, pCommonConfig->HttpPort);

    // 启动主循环，开始运行应用程序
    app.Run();

    // 程序结束时清理资源
    app.Dispose();

    return 0;
}
