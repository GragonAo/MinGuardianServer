#include "common.h"
#include "server_app.h"

#include "app_type.h"
#include "yaml.h"

#include "object_pool_packet.h"
#include "component_help.h"
#include "global.h"

// 构造函数，接收应用类型（appType）和命令行参数
ServerApp::ServerApp(APP_TYPE appType, int argc, char* argv[])
{
    _appType = appType; // 设置应用类型
    _argc = argc; // 命令行参数数量
    _argv = argv; // 命令行参数
}

// 资源释放函数，释放动态对象池和线程管理器
void ServerApp::Dispose()
{
    // 释放动态数据包对象池的资源并销毁实例
    DynamicPacketPool::GetInstance()->Dispose();
    DynamicPacketPool::DestroyInstance();

    // 销毁线程管理器实例
    ThreadMgr::DestroyInstance();
}

// 初始化函数，处理命令行参数、注册全局组件、启动线程
void ServerApp::Initialize()
{
    std::cout << "\ncommand arguments:" << std::endl;
    // 输出所有命令行参数
    for (auto count = 0; count < _argc; count++)
        std::cout << "  argv[" << count << "]   " << _argv[count] << std::endl;

    // 解析命令行参数，查找 -sid= 参数，设置应用程序 ID
    for (int argIdx = 1; argIdx < _argc; ++argIdx)
    {
        std::string cmd = _argv[argIdx];
        std::string findcmd = "-sid=";
        std::string::size_type fi1 = cmd.find(findcmd);
        if (fi1 != std::string::npos)
        {
            cmd.erase(fi1, findcmd.size());
            _appId = std::stoi(cmd); // 将命令行参数转换为整数，作为应用程序 ID
            break;
        }
    }

    // 注册信号处理函数，处理 SIGINT 信号
    signal(SIGINT, Signalhandler);

    // 初始化全局对象
    Global::Instance(_appType, _appId);

    // 初始化数据包对象池
    DynamicPacketPool::Instance();

    // 获取线程管理器实例并初始化全局组件
    _pThreadMgr = ThreadMgr::Instance();
    _pThreadMgr->InitializeGlobalComponent(_appType, _appId);

    // 初始化线程
    _pThreadMgr->InitializeThread();
}

// 信号处理函数，用于捕获和处理系统信号
void ServerApp::Signalhandler(const int signalValue)
{
    auto pGlobal = Global::GetInstance(); // 获取全局对象
    switch (signalValue)
    {
#if ENGINE_PLATFORM != PLATFORM_WIN32
    case SIGSTOP: // 处理 SIGSTOP 信号（非 Windows 平台）
    case SIGQUIT: // 处理 SIGQUIT 信号（非 Windows 平台）
#endif
    case SIGTERM: // 处理 SIGTERM 信号
    case SIGINT:  // 处理 SIGINT 信号
        pGlobal->IsStop = true; // 设置全局标志，指示需要停止应用程序
        break;
    }

    std::cout << "\nrecv signal. value:" << signalValue << " Global IsStop::" << pGlobal->IsStop << std::endl;
}

// 运行函数，包含服务器主循环逻辑
void ServerApp::Run()
{
    log4cplus::initialize(); // 初始化日志系统

    auto pGlobal = Global::GetInstance(); // 获取全局对象
    // 主循环，直到接收到停止信号
    while (!pGlobal->IsStop)
    {
        pGlobal->UpdateTime(); // 更新全局时间
        _pThreadMgr->Update(); // 更新线程管理器
        DynamicPacketPool::GetInstance()->Update(); // 更新动态数据包池
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // 休眠 1 毫秒，减少 CPU 使用率
    }

    // 退出时停止所有线程
    std::cout << "stoping all threads..." << std::endl;
    bool isStop;
    do
    {
        isStop = _pThreadMgr->IsStopAll(); // 检查是否所有线程都停止
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 休眠 10 毫秒
    } while (!isStop);

    // 销毁所有线程
    std::cout << "destroy all threads..." << std::endl;
    _pThreadMgr->DestroyThread(); // 销毁线程
    bool isDestroy;
    do
    {
        isDestroy = _pThreadMgr->IsDestroyAll(); // 检查是否所有线程都销毁
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 休眠 10 毫秒
    } while (!isDestroy);

    // 清理资源
    _pThreadMgr->Dispose();

    log4cplus::deinitialize(); // 关闭日志系统
}
