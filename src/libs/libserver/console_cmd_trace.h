#pragma once
#include "console.h"

// ConsoleCmdTrace 类，继承自 ConsoleCmd，用于处理追踪相关的控制台命令
class ConsoleCmdTrace : public ConsoleCmd
{
public:
    // 注册命令处理函数
    void RegisterHandler() override;
    
    // 显示帮助信息
    void HandleHelp() override;

private:
    // 处理连接相关的命令
    void HandleConnect(std::vector<std::string>& params);
    
    // 处理数据包相关的命令
    void HandlePacket(std::vector<std::string>& params);
    
    // 处理玩家相关的命令
    void HandlePlayer(std::vector<std::string>& params);
    
    // 处理账户相关的命令
    void HandleAccount(std::vector<std::string>& params);

    // 清理追踪信息的命令
    void HandleClean(std::vector<std::string>& params);
};
