#pragma once
#include "libserver/common.h"
#include "libserver/socket_object.h"
#include "libserver/entity.h"
#include "libserver/system.h"

// 玩家类，继承自 Entity 和 NetIdentify
class Player : public Entity<Player>, public NetIdentify,
               virtual public IAwakeFromPoolSystem<NetIdentify*, std::string>,
               virtual public IAwakeFromPoolSystem<NetIdentify*, uint64, uint64>
{
public:
    // 使用账户名初始化玩家
    void Awake(NetIdentify* pIdentify, std::string account) override;

    // 使用玩家序号和世界序号初始化玩家
    void Awake(NetIdentify* pIdentify, uint64 playerSn, uint64 worldSn) override;

    // 将玩家对象返回到对象池
    void BackToPool() override;

    // 获取账户名
    std::string GetAccount() const;

    // 获取玩家名
    std::string GetName() const;

    // 获取玩家序号
    uint64 GetPlayerSN() const;

    // 获取 Proto::Player 对象
    Proto::Player& GetPlayerProto();

    // 从流中解析数据
    void ParseFromStream(uint64 playerSn, std::stringstream* pOpStream);

    // 从 Proto 对象中解析数据
    void ParserFromProto(uint64 playerSn, const Proto::Player& proto);

    // 将玩家数据序列化到 Proto 对象中
    void SerializeToProto(Proto::Player* pProto) const;

protected:
    std::string _account{ "" };  // 玩家账户名
    std::string _name{ "" };     // 玩家名称

    uint64 _playerSn{ 0 };       // 玩家序号
    Proto::Player _player;       // 玩家数据的 Proto 对象
};
