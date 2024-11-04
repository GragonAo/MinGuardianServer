#include "console_cmd_world_proxy.h"  // 包含控制台命令类的头文件
#include "libserver/message_system_help.h"  // 包含消息系统帮助类

// 注册命令处理器
void ConsoleCmdWorldProxy::RegisterHandler()
{
    // 注册处理命令 "-all"，并绑定到 HandleShowAllWorld 方法
    OnRegisterHandler("-all", BindFunP1(this, &ConsoleCmdWorldProxy::HandleShowAllWorld));
}

// 处理帮助命令，输出帮助信息
void ConsoleCmdWorldProxy::HandleHelp()
{
    // 输出命令的帮助信息
    std::cout << "\t-all.\t\tshow all proxyworlds" << std::endl;
}

// 处理显示所有世界的命令
void ConsoleCmdWorldProxy::HandleShowAllWorld(std::vector<std::string>& params)
{
    // 发送命令请求，通知系统显示所有世界代理的信息
    MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_CmdWorldProxy, 0);
}
