#pragma once
#include "libserver/console.h"

// 控制台命令类，用于处理创建相关的命令
class ConsoleCmdCreate : public ConsoleCmd
{
public:
    // 注册命令处理器
    void RegisterHandler() override;

    // 处理帮助命令，提供命令的使用说明
    void HandleHelp() override;

protected:
    // 处理显示所有世界的命令
    void HandleShowAllWorld(std::vector<std::string>& params);
};
