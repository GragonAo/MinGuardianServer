#pragma once

#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <map>
#include <mutex>
#include <string>

#include "system.h"
#include "disposable.h"
#include "entity.h"

#define ConsoleMaxBuffer 512

// 定义控制台命令处理的回调类型，接收字符串参数列表
typedef std::function<void(std::vector<std::string>&)> HandleConsole;

// ConsoleCmd 类用于管理和处理控制台命令，继承自 IDisposable 接口
class ConsoleCmd : public IDisposable
{
public:
    // 注册命令处理器，子类需要实现该方法
    virtual void RegisterHandler() = 0;

    // 显示帮助信息，子类需要实现该方法
    virtual void HandleHelp() = 0;

    // 释放资源方法的实现，来自 IDisposable 接口
    void Dispose() override;

    // 处理控制台输入的命令
    void Process(std::vector<std::string>& params);

protected:
    // 注册一个控制台命令的处理器，key 是命令字符串，handler 是处理函数
    void OnRegisterHandler(std::string key, HandleConsole handler);

    // 检查命令的参数数量是否符合要求
    static bool CheckParamCnt(std::vector<std::string>& params, const size_t count);

private:
    // 存储命令与对应处理器的映射
    std::map<std::string, HandleConsole> _handles;
};

// Console 类用于管理整个控制台系统，继承自 Entity 和 IAwakeSystem
class Console : public Entity<Console>, public IAwakeSystem<>
{
public:
    // 实现 IAwakeSystem 接口的 Awake 方法，用于初始化控制台
    void Awake() override;

    // 回收资源，将对象重置为初始状态
    void BackToPool() override;

    // 更新控制台，处理输入的命令
    void Update();

    // 模板函数，用于注册一个控制台命令处理类
    template<class T>
    void Register(std::string cmd);

protected:
    // 存储命令字符串与对应的 ConsoleCmd 对象的映射
    std::map<std::string, ConsoleCmd*> _handles;

    // 用于保护命令队列的互斥锁
    std::mutex _lock;

    // 控制台输入处理的线程
    std::thread _thread;

    // 存储用户输入的命令队列
    std::queue<std::string> _commands;

    // 表示控制台是否在运行
    bool _isRun{ true };
};

// 模板函数的实现，注册并初始化一个控制台命令处理类
template<class T>
void Console::Register(std::string cmd)
{
    // 创建命令处理对象并注册命令处理器
    T* pObj = new T();
    pObj->RegisterHandler();

    // 将命令与处理器映射存储到 _handles 容器中
    this->_handles[cmd] = pObj;
}
