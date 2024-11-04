#pragma once
#include "disposable.h"
#include "common.h"
#include "thread_mgr.h"
#include "app_type.h"

#if ENGINE_PLATFORM != PLATFORM_WIN32
#include <signal.h> // 非 Windows 平台的信号处理头文件
#else
#include <csignal>  // Windows 平台的信号处理头文件
#endif

// ServerApp 类：负责服务器应用程序的初始化、运行和信号处理
class ServerApp :public Singleton<ServerApp>, public IDisposable
{
public:
    // 构造函数，接收应用类型和命令行参数
    ServerApp(APP_TYPE appType, int argc, char* argv[]);

    // 初始化函数，用于设置相关资源
    void Initialize();
    
    // 释放资源函数，重载 IDisposable 接口的 Dispose 函数
    void Dispose() override;

    // 运行函数，启动应用程序
    void Run();

    // 静态信号处理函数，用于捕捉和处理系统信号
    static void Signalhandler(int signalValue);

protected:
    ThreadMgr* _pThreadMgr{ nullptr };  // 线程管理器指针
    APP_TYPE _appType{ APP_TYPE::APP_None };  // 应用类型，初始化为 APP_None
    int _appId{ 0 };  // 应用 ID

    int _argc;  // 命令行参数数量
    char** _argv;  // 命令行参数
};
