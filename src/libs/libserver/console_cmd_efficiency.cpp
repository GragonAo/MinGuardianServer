#include "console_cmd_efficiency.h"
#include "message_system_help.h"

// 注册命令处理器
void ConsoleCmdEfficiency::RegisterHandler()
{
    // 注册 "-thread" 命令，并绑定处理函数 HandleThread
    OnRegisterHandler("-thread", BindFunP1(this, &ConsoleCmdEfficiency::HandleThread));
}

// 显示帮助信息
void ConsoleCmdEfficiency::HandleHelp()
{
    // 输出关于 "-thread" 命令的说明
    std::cout << "\t-thread.\n\t\tthread info" << std::endl;
}

// 处理 "-thread" 命令
void ConsoleCmdEfficiency::HandleThread(std::vector<std::string>& params)
{
    // 发送请求包以获取线程效率信息
    MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_CmdEfficiency, 0);
}
