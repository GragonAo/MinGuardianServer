#include "console_cmd_thread.h"
#include "message_system_help.h"
#include "common.h"
#include "object_pool_packet.h"

// 注册命令处理函数，将命令字符串与对应的处理方法绑定
void ConsoleCmdThread::RegisterHandler()
{
    // 处理 "-entity" 命令，显示线程中的实体信息
    OnRegisterHandler("-entity", BindFunP1(this, &ConsoleCmdThread::HandleEntity));
    // 处理 "-pool" 命令，显示线程中的对象池信息
    OnRegisterHandler("-pool", BindFunP1(this, &ConsoleCmdThread::HandlePool));
    // 处理 "-connect" 命令，显示线程中的连接信息
    OnRegisterHandler("-connect", BindFunP1(this, &ConsoleCmdThread::HandleConnect));
}

// 显示帮助信息，介绍各命令的使用方式
void ConsoleCmdThread::HandleHelp()
{
    std::cout << "\t-entity.    显示线程中的实体信息。" << std::endl;
    std::cout << "\t-pool.      显示线程中的对象池信息。" << std::endl;
    std::cout << "\t-connect.   显示线程中的连接信息。" << std::endl;
}

// 处理 "-entity" 命令，向系统派发显示实体信息的消息
void ConsoleCmdThread::HandleEntity(std::vector<std::string>& params)
{
    Proto::CmdThread cmdProto;
    cmdProto.set_cmd_type(Proto::CmdThread_CmdType_Entity);
    MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_CmdThread, cmdProto, nullptr);
}

// 处理 "-pool" 命令，显示对象池的调试信息并派发相关消息
void ConsoleCmdThread::HandlePool(std::vector<std::string>& params)
{
    LOG_DEBUG("------------------------------------");
    // 显示动态对象池的当前状态
    DynamicPacketPool::GetInstance()->Show();

    Proto::CmdThread cmdProto;
    cmdProto.set_cmd_type(Proto::CmdThread_CmdType_Pool);
    MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_CmdThread, cmdProto, nullptr);
}

// 处理 "-connect" 命令，向系统派发显示连接信息的消息
void ConsoleCmdThread::HandleConnect(std::vector<std::string>& params)
{
    Proto::CmdThread cmdProto;
    cmdProto.set_cmd_type(Proto::CmdThread_CmdType_Connect);
    MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_CmdThread, cmdProto, nullptr);
}
