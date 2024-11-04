#pragma once

#include "libserver/console.h"  // 控制台命令基类

// ConsoleCmdWorldProxy 类用于处理与世界代理相关的控制台命令
class ConsoleCmdWorldProxy : public ConsoleCmd
{
public:
    // 注册命令处理器，通常在此方法中定义命令及其对应的处理函数
    void RegisterHandler() override;
    
    // 处理帮助命令，通常显示帮助信息
    void HandleHelp() override;

protected:
    // 处理显示所有世界的命令
    void HandleShowAllWorld(std::vector<std::string>& params);
};
