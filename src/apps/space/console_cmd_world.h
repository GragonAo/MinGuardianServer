#pragma once // 防止头文件被多次包含

#include "libserver/console.h" // 包含控制台命令的基类定义

// 定义 ConsoleCmdWorld 类，继承自 ConsoleCmd
class ConsoleCmdWorld : public ConsoleCmd
{
public:
    // 重写 RegisterHandler 方法，用于注册控制台命令的处理程序
    void RegisterHandler() override;

    // 重写 HandleHelp 方法，用于处理帮助命令
    void HandleHelp() override;

protected:
    // 处理显示所有世界的命令
    void HandleShowAllWorld(std::vector<std::string>& params);
};
