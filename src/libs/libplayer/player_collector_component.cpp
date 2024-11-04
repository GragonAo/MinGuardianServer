#include "player_collector_component.h"
#include "player.h"
#include "libserver/message_system_help.h"

void PlayerCollectorComponent::Awake()
{
    // 在这里可以初始化组件，当前未实现任何功能
}

void PlayerCollectorComponent::BackToPool()
{
    auto pEntitySystem = GetSystemManager()->GetEntitySystem();
    
    // 移除所有玩家组件并释放资源
    for (auto iter = _players.begin(); iter != _players.end();)
    {
        pEntitySystem->RemoveComponent(iter->second); // 从实体系统中移除玩家
        iter = _players.erase(iter); // 迭代器在移除后需要更新
    }

    // 清空账号映射
    _accounts.clear();
}

Player* PlayerCollectorComponent::AddPlayer(NetIdentify* pIdentify, const std::string account)
{
    const auto socket = pIdentify->GetSocketKey()->Socket; // 获取玩家的 socket

    // 检查玩家是否已存在
    if (_players.find(socket) != _players.end())
    {
        std::cout << "AddPlayer error: Player already exists." << std::endl;
        return nullptr; // 如果已存在，返回 nullptr
    }

    _accounts[account] = socket; // 将账号和 socket 关联
    const auto pPlayer = GetSystemManager()->GetEntitySystem()->AddComponent<Player>(pIdentify, account); // 创建玩家组件
    _players[socket] = pPlayer; // 将玩家添加到集合中
    return pPlayer; // 返回新添加的玩家
}

void PlayerCollectorComponent::RemovePlayer(Player* pPlayer)
{
    // 移除玩家并释放相关资源
    _players.erase(pPlayer->GetSocketKey()->Socket); // 从玩家集合中移除
    _accounts.erase(pPlayer->GetAccount()); // 从账号集合中移除
    GetSystemManager()->GetEntitySystem()->RemoveComponent(pPlayer); // 从实体系统中移除玩家组件
}

void PlayerCollectorComponent::RemovePlayerBySocket(SOCKET socket)
{
    auto iter = _players.find(socket); // 查找指定 socket 的玩家
    if (iter != _players.end())
    {
        RemovePlayer(iter->second); // 如果找到，移除该玩家
    }
}

void PlayerCollectorComponent::RemovePlayerBySn(uint64 playerSn)
{
    // 根据玩家序列号查找并移除玩家
    auto iter = std::find_if(_players.begin(), _players.end(), [&playerSn](auto pair)
    {
        return pair.second->GetPlayerSN() == playerSn; // 匹配玩家序列号
    });

    if (iter != _players.end())
    {
        RemovePlayer(iter->second); // 找到后移除玩家
    }
}

void PlayerCollectorComponent::RemoveAllPlayerAndCloseConnect()
{
    // 移除所有玩家并发送断开连接请求
    while (!_players.empty())
    {
        auto iter = _players.begin();
        const auto pPlayer = iter->second;

        // 发送断开连接的消息
        MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_NetworkRequestDisconnect, pPlayer);
        RemovePlayer(pPlayer); // 移除玩家
    }    
}

Player* PlayerCollectorComponent::GetPlayerBySocket(const SOCKET socket)
{
    // 根据 socket 查找玩家
    auto iter = _players.find(socket);
    return (iter != _players.end()) ? iter->second : nullptr; // 找到则返回玩家，否则返回 nullptr
}

Player* PlayerCollectorComponent::GetPlayerByAccount(const std::string account)
{
    // 根据账号查找玩家
    auto iter = _accounts.find(account);
    if (iter != _accounts.end())
    {
        SOCKET socket = iter->second;
        auto iterPlayer = _players.find(socket);
        if (iterPlayer != _players.end())
        {
            return iterPlayer->second; // 找到则返回玩家
        }
        // 如果玩家未找到，清除失效的账号映射
        _accounts.erase(account);
    }
    return nullptr; // 未找到则返回 nullptr
}

Player* PlayerCollectorComponent::GetPlayerBySn(uint64 playerSn)
{
    // 根据玩家序列号查找玩家
    auto iter = std::find_if(_players.begin(), _players.end(), [&playerSn](auto pair)
    {
        return pair.second->GetPlayerSN() == playerSn; // 匹配玩家序列号
    });

    return (iter != _players.end()) ? iter->second : nullptr; // 找到则返回玩家，否则返回 nullptr
}

int PlayerCollectorComponent::OnlineSize() const
{
    return _players.size(); // 返回当前在线玩家的数量
}

std::map<SOCKET, Player*>& PlayerCollectorComponent::GetAll()
{
    return _players; // 返回所有玩家的集合
}
