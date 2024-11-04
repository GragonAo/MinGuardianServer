#include "console.h"

#include <iostream>
#include <thread>
#include <mutex>
#include <map>
#include <string>
#include <queue>
#include <vector>

#include "util_string.h"
#include "log4_help.h"
#include "update_component.h"
#include "component_help.h"
#include "global.h"

// 注册控制台命令处理器
void ConsoleCmd::OnRegisterHandler(std::string key, HandleConsole handler)
{
    _handles[key] = handler;
}

// 检查输入的参数数量是否符合要求
bool ConsoleCmd::CheckParamCnt(std::vector<std::string>& params, const size_t count)
{
    if (params.size() == count)
        return true;

    std::cout << "输入的参数数量错误，请查看帮助: -help" << std::endl;
    return false;
}

// 释放命令处理器资源
void ConsoleCmd::Dispose()
{
    _handles.clear();
}

// 处理命令输入
void ConsoleCmd::Process(std::vector<std::string>& params)
{
    if (params.size() <= 1)
        return;

    const std::string key = params[1];
    const auto iter = _handles.find(key);

    if (iter == _handles.end())
    {
        // 如果命令找不到，检查是否输入了帮助命令
        if (key == "-help")
        {
            HandleHelp();
            return;
        }

        LOG_ERROR("输入错误，未找到命令: " << key.c_str());
        return;
    }

    // 移除前两个参数 (命令和子命令)
    params.erase(params.begin(), params.begin() + 2);
    iter->second(params);
}

///////////////////////////////////////////////////////////////////

// 初始化控制台并启动输入监听线程
void Console::Awake()
{
    // 启动新的线程，用于监听用户的控制台输入
    _thread = std::thread([this]()
        {
            char _buffer[ConsoleMaxBuffer];
            do
            {
                std::cin.getline(_buffer, ConsoleMaxBuffer);

                _lock.lock();
                _commands.push(std::string(_buffer));
                _lock.unlock();

                std::string cmdMsg = _buffer;
                auto iter = cmdMsg.find("-exit");
                if (iter != std::string::npos)
                {
                    // 如果检测到输入 "-exit"，则停止控制台
                    Global::GetInstance()->IsStop = true;
                    _isRun = false;
                }

            } while (_isRun);
        });

    // 添加更新组件，用于定时处理命令队列中的命令
    AddComponent<UpdateComponent>(BindFunP0(this, &Console::Update));
}

// 回收控制台资源
void Console::BackToPool()
{
    for (auto& pair : _handles)
    {
        pair.second->Dispose();
        delete pair.second;
    }
    _handles.clear();

    // 处理线程的结束
    if (_isRun)
        _thread.detach();
    else
        _thread.join();
}

// 控制台的更新方法，用于处理输入的命令
void Console::Update()
{
    _lock.lock();
    if (_commands.empty())
    {
        _lock.unlock();
        return;
    }

    const std::string cmd = _commands.front();
    _commands.pop();
    _lock.unlock();

    // 分割输入的命令，转换为参数列表
    std::vector<std::string> params;
    strutil::split(cmd, ' ', params);
    if (params.empty())
        return;

    const std::string key = params[0];
    auto iter = _handles.find(key);
    if (iter == _handles.end())
    {
        // 如果输入 "help"，显示所有可用的命令和帮助信息
        if (key == "help")
        {
            iter = _handles.begin();
            while (iter != _handles.end())
            {
                std::cout << "\r\n" << iter->first.c_str() << std::endl;
                iter->second->HandleHelp();
                ++iter;
            }
            return;
        }

        LOG_ERROR("输入错误，未找到命令: " << cmd.c_str());
        return;
    }

    // 执行对应命令的处理函数
    iter->second->Process(params);
}
