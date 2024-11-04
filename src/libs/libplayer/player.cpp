#include "player.h"
#include "player_component.h"
#include "libserver/message_system_help.h"

// 玩家对象的初始化方法，使用账户名
void Player::Awake(NetIdentify* pIdentify, std::string account)
{
    _account = account;  // 保存账户名
    _playerSn = 0;      // 玩家序号初始化为0
    _player.Clear();     // 清空玩家数据

    if (pIdentify != nullptr)
        _socketKey.CopyFrom(pIdentify->GetSocketKey());  // 从网络标识中复制 socketKey

    _tagKey.Clear();  // 清空标签键
    _tagKey.AddTag(TagType::Account, _account);  // 添加账户标签

    // 记录成功后修改网络监听的标识
    MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_NetworkListenKey, this);
}

// 玩家对象的初始化方法，使用玩家序号和世界序号
void Player::Awake(NetIdentify* pIdentify, uint64 playerSn, uint64 worldSn)
{
    _account = "";  // 账户名清空
    _playerSn = playerSn;  // 设置玩家序号
    _player.Clear();  // 清空玩家数据

    if (pIdentify != nullptr)
        _socketKey.CopyFrom(pIdentify->GetSocketKey());  // 从网络标识中复制 socketKey

    _tagKey.Clear();  // 清空标签键
    _tagKey.AddTag(TagType::Player, playerSn);  // 添加玩家标签
    _tagKey.AddTag(TagType::Entity, worldSn);  // 添加世界实体标签

    // 记录成功后修改网络监听的标识
    //MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_NetworkListenKey, this);
}

// 将玩家对象返回到对象池
void Player::BackToPool()
{
    _account = "";  // 账户名清空
    _name = "";     // 玩家名清空
    _playerSn = 0;  // 玩家序号重置为0
    _player.Clear(); // 清空玩家数据

    _socketKey.Clear();  // 清空 socketKey
    _tagKey.Clear();     // 清空标签键
}

// 获取账户名
std::string Player::GetAccount() const
{
    return _account;
}

// 获取玩家名
std::string Player::GetName() const
{
    return _name;
}

// 获取玩家序号
uint64 Player::GetPlayerSN() const
{
    return _playerSn;
}

// 获取 Proto::Player 对象
Proto::Player& Player::GetPlayerProto()
{
    return _player;
}

// 从流中解析数据
void Player::ParseFromStream(const uint64 playerSn, std::stringstream* pOpStream)
{
    _playerSn = playerSn;  // 设置玩家序号
    _player.ParseFromIstream(pOpStream);  // 从输入流中解析玩家数据
}

// 从 Proto 对象中解析数据
void Player::ParserFromProto(const uint64 playerSn, const Proto::Player& proto)
{
    _playerSn = playerSn;  // 设置玩家序号
    _player.CopyFrom(proto);  // 从 proto 复制玩家数据
    _name = _player.name();  // 获取玩家名称

    // 遍历并更新组件
    for (auto pair : _components)
    {
        auto pPlayerComponent = dynamic_cast<PlayerComponent*>(pair.second);
        if (pPlayerComponent == nullptr)
            continue;  // 如果不是 PlayerComponent，则跳过

        pPlayerComponent->ParserFromProto(proto);  // 从 proto 更新组件
    }
}

// 将玩家数据序列化到 Proto 对象中
void Player::SerializeToProto(Proto::Player* pProto) const
{
    // 复制玩家数据
    pProto->CopyFrom(_player);

    // 遍历并序列化组件
    for (auto pair : _components)
    {
        auto pPlayerComponent = dynamic_cast<PlayerComponent*>(pair.second);
        if (pPlayerComponent == nullptr)
            continue;  // 如果不是 PlayerComponent，则跳过

        pPlayerComponent->SerializeToProto(pProto);  // 将组件数据序列化到 proto
    }
}
