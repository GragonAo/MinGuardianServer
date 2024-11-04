#pragma once
#include "console.h"

// ConsoleCmdThread 类用于处理与线程相关的控制台命令
class ConsoleCmdThread : public ConsoleCmd
{
public:
    // 注册命令处理函数
    void RegisterHandler() override;
    // 显示帮助信息
    void HandleHelp() override;

private:
    // 处理 "-entity" 命令，显示实体信息
    void HandleEntity(std::vector<std::string>& params);
    // 处理 "-pool" 命令，显示线程池信息
    void HandlePool(std::vector<std::string>& params);
    // 处理 "-connect" 命令，显示连接信息
    void HandleConnect(std::vector<std::string>& params);
};
