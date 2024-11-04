#include "trace_component.h"
#include "global.h"
#include "log4_help.h"

// 在追踪详情中添加一条追踪记录
void TraceDetail::Trace(const std::string& trace)
{
    // 获取当前时间戳并将其与追踪信息组合成字符串，添加到追踪详情列表中
    _details.emplace_back(timeutil::ToString(Global::GetInstance()->TimeTick) + " " + trace);
}

// 输出所有追踪记录
void TraceDetail::Show()
{
    // 遍历追踪详情列表，逐条输出追踪信息
    for (auto one : _details)
    {
        std::cout << one << std::endl;
    }
}

// TraceComponent 的初始化函数，当前没有额外的初始化操作
void TraceComponent::Awake()
{

}

// 重置 TraceComponent，将追踪信息清空
void TraceComponent::BackToPool()
{
    // 清空存储追踪信息的 map
    _traces.clear();
}

// 记录某个账户与 socket 的关联关系
void TraceComponent::TraceAccount(std::string account, SOCKET socket)
{
    std::lock_guard<std::mutex> guard(_lock); // 加锁，确保线程安全

    // 查找账户是否已经存在于 _accounts 中
    const auto iter = _accounts.find(account);
    if (iter == _accounts.end())
    {
        // 如果账户不存在，创建一个新的账户并关联一个空的 socket 集合
        _accounts.insert(std::make_pair(account, std::set<SOCKET>()));
    }

    // 将 socket 插入到对应账户的 socket 集合中
    _accounts[account].insert(socket);
}

// 显示某个账户关联的所有 socket
void TraceComponent::ShowAccount(const std::string& account)
{
    std::lock_guard<std::mutex> guard(_lock); // 加锁，确保线程安全

    // 查找该账户对应的 socket 集合
    const auto iter = _accounts.find(account);
    if (iter == _accounts.end())
    {
        // 如果账户未找到，输出警告日志
        LOG_WARN("not found. account:" << account.c_str());
        return;
    }

    // 输出该账户对应的所有 socket
    std::cout << "socket:";
    for (auto one : iter->second)
    {
        std::cout << " " << one;
    }

    std::cout << std::endl;
}

// 记录指定类型和 key 的追踪信息
void TraceComponent::Trace(TraceType iType, int key, const std::string& trace)
{
    std::lock_guard<std::mutex> guard(_lock); // 加锁，确保线程安全

    // 查找指定类型的追踪信息，如果没有找到则创建一个新的条目
    const auto iter = _traces.find(iType);
    if (iter == _traces.end())
    {
        _traces.insert(std::make_pair(iType, std::map<int, TraceDetail>()));
    }

    // 查找指定 key 的追踪详情，如果没有找到则创建新的追踪详情
    const auto iterKey = _traces[iType].find(key);
    if (iterKey == _traces[iType].end())
    {
        _traces[iType].insert(std::make_pair(key, TraceDetail()));
    }

    // 添加追踪信息到指定 key 的追踪详情中
    _traces[iType][key].Trace(trace);

    // 可以在特定条件下打印日志（如连接器或玩家的追踪信息），目前该代码被注释掉
    //if (iType == TraceType::Connector || iType == TraceType::Player)
    //    LOG_DEBUG("key:" << key << " " << trace.c_str());
}

// 显示指定类型和 key 的追踪信息
void TraceComponent::Show(TraceType iType, int key)
{
    std::lock_guard<std::mutex> guard(_lock); // 加锁，确保线程安全

    // 查找指定类型的追踪信息
    const auto iter = _traces.find(iType);
    if (iter == _traces.end())
        return;

    // 查找指定 key 的追踪详情
    auto keyTrace = _traces[iType];
    auto iterKey = keyTrace.find(key);
    if (iterKey == keyTrace.end())
        return;

    // 输出 key 以及其对应的追踪详情
    std::cout << " key:" << iterKey->first << std::endl;
    iterKey->second.Show();
}

// 清空所有追踪信息
void TraceComponent::Clean()
{
    std::lock_guard<std::mutex> guard(_lock); // 加锁，确保线程安全
    _traces.clear(); // 清空追踪信息的 map
}
