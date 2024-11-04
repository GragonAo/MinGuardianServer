#pragma once
#include "console.h"

class ConsoleCmdEfficiency : public ConsoleCmd
{
public:
    void RegisterHandler() override; // 注册命令处理器
    void HandleHelp() override; // 处理帮助信息

private:
    void HandleThread(std::vector<std::string>& params); // 处理线程相关命令
};
