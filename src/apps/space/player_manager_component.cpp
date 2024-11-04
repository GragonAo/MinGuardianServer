#include "player_manager_component.h"           // 包含 PlayerManagerComponent 的头文件
#include "libserver/message_system_help.h"       // 包含消息系统帮助的头文件

// Awake 方法在组件被激活时调用
void PlayerManagerComponent::Awake()
{
    // 初始化时不需要任何操作
}

// BackToPool 方法在组件被返回到对象池时调用
void PlayerManagerComponent::BackToPool()
{
    // 如果有玩家存在于管理器中，记录错误并抛出异常
    if (!_players.empty())
    {
        LOG_ERROR("called PlayerManagerComponent BackToPool. but it has player.");
    }

    // 清空玩家列表
    _players.clear();
}

// 添加玩家到管理器
Player* PlayerManagerComponent::AddPlayer(const uint64 playerSn, uint64 worldSn, NetIdentify* pNetIdentify)
{
    // 检查玩家是否已经存在
    const auto iter = _players.find(playerSn);
    if (iter != _players.end())
    {
        std::cout << "AddPlayer error." << std::endl;
        return nullptr; // 如果存在则返回 nullptr
    }

    // 添加新玩家
    const auto pPlayer = GetSystemManager()->GetEntitySystem()->AddComponent<Player>(pNetIdentify, playerSn, worldSn);
    _players[playerSn] = pPlayer; // 将玩家添加到玩家列表中
    return pPlayer; // 返回新添加的玩家指针
}

// 根据玩家 SN 获取玩家
Player* PlayerManagerComponent::GetPlayerBySn(const uint64 playerSn)
{
    // 查找玩家
    const auto iter = _players.find(playerSn);
    if (iter == _players.end())
        return nullptr; // 如果未找到则返回 nullptr

    return iter->second; // 返回找到的玩家指针
}

// 根据玩家 SN 移除玩家
void PlayerManagerComponent::RemovePlayerBySn(const uint64 playerSn)
{
    // 查找玩家
    const auto iter = _players.find(playerSn);
    if (iter == _players.end())
        return; // 如果未找到则直接返回

    Player* pPlayer = iter->second; // 获取玩家指针
    _players.erase(playerSn); // 从管理器中移除玩家

    // 移除玩家组件
    GetSystemManager()->GetEntitySystem()->RemoveComponent(pPlayer);
}

// 移除所有与指定网络标识相匹配的玩家
void PlayerManagerComponent::RemoveAllPlayers(NetIdentify* pNetIdentify)
{
    auto iter = _players.begin();
    while (iter != _players.end())
    {
        auto pPlayer = iter->second; // 获取当前玩家
        // 如果玩家的 Socket 不匹配，则继续下一个玩家
        if (pPlayer->GetSocketKey()->Socket != pNetIdentify->GetSocketKey()->Socket)
        {
            ++iter;
            continue;
        }

        iter = _players.erase(iter); // 移除玩家

        // 保存玩家数据
        Proto::SavePlayer protoSave;
        protoSave.set_player_sn(pPlayer->GetPlayerSN()); // 设置玩家 SN
        pPlayer->SerializeToProto(protoSave.mutable_player()); // 序列化玩家数据
        MessageSystemHelp::SendPacket(Proto::MsgId::G2DB_SavePlayer, protoSave, APP_DB_MGR); // 发送保存消息

        // 移除玩家组件
        GetSystemManager()->GetEntitySystem()->RemoveComponent(pPlayer);
    }
}

// 获取当前在线玩家数量
int PlayerManagerComponent::OnlineSize() const
{
    return _players.size(); // 返回玩家数量
}

// 获取所有玩家
std::map<uint64, Player*>* PlayerManagerComponent::GetAll()
{
    return &_players; // 返回玩家列表的指针
}
