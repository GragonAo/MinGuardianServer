#include "console_cmd_world.h" // 包含控制台命令世界的头文件
#include "libserver/message_system_help.h" // 包含消息系统帮助的头文件

// 注册控制台命令的处理程序
void ConsoleCmdWorld::RegisterHandler()
{
    // 将命令 "-all" 绑定到处理函数 HandleShowAllWorld
    OnRegisterHandler("-all", BindFunP1(this, &ConsoleCmdWorld::HandleShowAllWorld));
}

// 处理帮助命令，显示可用的控制台命令及其说明
void ConsoleCmdWorld::HandleHelp()
{
    // 输出帮助信息，说明 "-all" 命令的作用
    std::cout << "\t-all.\t\tshow all worlds" << std::endl;
}

// 处理显示所有世界的命令
void ConsoleCmdWorld::HandleShowAllWorld(std::vector<std::string>& params)
{
    // 通过消息系统发送一个包，表示请求显示所有世界
    MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_CmdWorld, nullptr);
}
