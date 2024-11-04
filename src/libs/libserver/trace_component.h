#pragma once
#include "component.h"
#include "system.h"
#include "common.h"

#include <map>
#include <set>

#if true
#define LOG_TRACE_COMPONENT_OPEN 1  // 宏定义，开启日志追踪功能
#endif

// 定义追踪类型的枚举类，区分不同的追踪信息
enum class TraceType
{
    Packet = 0,    // 数据包追踪
    Connector = 1, // 连接器追踪
    Player = 2,    // 玩家追踪
    Time = 3,      //耗时追踪
};

// 追踪详情类，用于存储和展示追踪信息
class TraceDetail
{
public:
    // 记录一条追踪信息
    void Trace(const std::string& trace);
    
    // 显示所有追踪信息
    void Show();

private:
    // 使用链表存储追踪详情
    std::list<std::string> _details;
};

// 追踪组件类，用于记录、显示和管理追踪信息
class TraceComponent : public Component<TraceComponent>, public IAwakeSystem<>
{
public:
    // 系统启动时调用，用于初始化组件
    void Awake() override;
    
    // 将组件恢复到对象池中，进行数据清理
    void BackToPool() override;

    // 记录一个账户和其对应的 socket
    void TraceAccount(std::string account, SOCKET socket);
    
    // 显示某个账户对应的 socket 信息
    void ShowAccount(const std::string& account);

    // 记录指定类型和 key 的追踪信息
    void Trace(TraceType iType, int key, const std::string& trace);
    
    // 显示指定类型和 key 的追踪信息
    void Show(TraceType iType, int key);
    
    // 清理所有追踪信息和账户信息
    void Clean();

private:
    std::mutex _lock;  // 互斥锁，用于多线程环境下保证数据的安全访问
    // 存储追踪信息的多级 map，第一层是 TraceType，第二层是对应的 key 和追踪详情
    std::map<TraceType, std::map<int, TraceDetail>> _traces;

    // 存储账户和其对应的 socket 集合
    std::map<std::string, std::set<SOCKET>> _accounts;
};
