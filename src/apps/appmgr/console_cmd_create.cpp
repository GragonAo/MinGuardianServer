#include "console_cmd_create.h"
#include "libserver/message_system_help.h"

// 注册命令处理器
void ConsoleCmdCreate::RegisterHandler()
{
    // 绑定 "-all" 命令到 HandleShowAllWorld 方法
    OnRegisterHandler("-all", BindFunP1(this, &ConsoleCmdCreate::HandleShowAllWorld));
}

// 显示帮助信息
void ConsoleCmdCreate::HandleHelp()
{
    std::cout << "\t-all.\t\tshow all worlds" << std::endl;  // 输出命令的帮助信息
}

// 处理显示所有世界的命令
void ConsoleCmdCreate::HandleShowAllWorld(std::vector<std::string>& params)
{
    // 调用 MessageSystemHelp 的 DispatchPacket 方法，发送显示所有世界的请求
    MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_CmdCreate, nullptr);
}
