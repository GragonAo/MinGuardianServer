#include "console_cmd_trace.h"
#include "component_help.h"

// 注册命令处理函数
void ConsoleCmdTrace::RegisterHandler()
{
    // 注册处理 "-connect" 命令的处理函数
    OnRegisterHandler("-connect", BindFunP1(this, &ConsoleCmdTrace::HandleConnect));
    // 注册处理 "-packet" 命令的处理函数
    OnRegisterHandler("-packet", BindFunP1(this, &ConsoleCmdTrace::HandlePacket));
    // 注册处理 "-player" 命令的处理函数
    OnRegisterHandler("-player", BindFunP1(this, &ConsoleCmdTrace::HandlePlayer));
    // 注册处理 "-account" 命令的处理函数
    OnRegisterHandler("-account", BindFunP1(this, &ConsoleCmdTrace::HandleAccount));

    // 注册处理 "-clean" 命令的处理函数，用于清理数据
    OnRegisterHandler("-clean", BindFunP1(this, &ConsoleCmdTrace::HandleClean));
}

// 显示命令的帮助信息
void ConsoleCmdTrace::HandleHelp()
{
    std::cout << "\t-connect socket.  \t\tshow the information of socket." << std::endl;
    std::cout << "\t-packet socket.  \t\tall trace of packet related socket." << std::endl;
    std::cout << "\t-player socket.  \t\tall trace of player related socket." << std::endl;
    std::cout << "\t-account account.  \t\tall trace of account." << std::endl;
    std::cout << "\t-clean.  \t\tclean all data." << std::endl;
}

// 处理 "-connect" 命令，显示指定 socket 的连接信息
void ConsoleCmdTrace::HandleConnect(std::vector<std::string>& params)
{
    // 检查参数数量是否正确
    if (!CheckParamCnt(params, 1))
        return;

    // 将参数转换为 socket 数值
    const int socket = std::atoi(params[0].c_str());
    
    // 获取追踪组件并显示连接信息
    auto pTraceComponent = ComponentHelp::GetTraceComponent();
    pTraceComponent->Show(TraceType::Connector, socket);
}

// 处理 "-packet" 命令，显示指定 socket 的数据包追踪信息
void ConsoleCmdTrace::HandlePacket(std::vector<std::string>& params)
{
    if (!CheckParamCnt(params, 1))
        return;

    const int socket = std::atoi(params[0].c_str());
    auto pTraceComponent = ComponentHelp::GetTraceComponent();
    pTraceComponent->Show(TraceType::Packet, socket);
}

// 处理 "-player" 命令，显示指定 socket 的玩家追踪信息
void ConsoleCmdTrace::HandlePlayer(std::vector<std::string>& params)
{
    if (!CheckParamCnt(params, 1))
        return;

    const int socket = std::atoi(params[0].c_str());
    auto pTraceComponent = ComponentHelp::GetTraceComponent();
    pTraceComponent->Show(TraceType::Player, socket);
}

// 处理 "-account" 命令，显示指定账户的追踪信息
void ConsoleCmdTrace::HandleAccount(std::vector<std::string>& params)
{
    if (!CheckParamCnt(params, 1))
        return;

    auto account = params[0];
    auto pTraceComponent = ComponentHelp::GetTraceComponent();
    pTraceComponent->ShowAccount(account);
}

// 处理 "-clean" 命令，清理所有追踪数据
void ConsoleCmdTrace::HandleClean(std::vector<std::string>& params)
{
    auto pTraceComponent = ComponentHelp::GetTraceComponent();
    pTraceComponent->Clean();
}
