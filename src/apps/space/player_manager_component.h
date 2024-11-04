#pragma once
#include "libserver/entity.h"       // 引入 Entity 类的头文件

#include "libplayer/player.h"        // 引入 Player 类的头文件

// PlayerManagerComponent 负责管理游戏中的玩家
class PlayerManagerComponent : public Entity<PlayerManagerComponent>, public IAwakeFromPoolSystem<>
{
public:
    // 组件激活时的初始化
    void Awake() override;

    // 组件返回到对象池时的清理
    void BackToPool() override;

    // 添加玩家到管理器
    Player* AddPlayer(uint64 playerSn, uint64 worldSn, NetIdentify* pNetIdentify);

    // 根据玩家 SN 获取玩家对象
    Player* GetPlayerBySn(uint64 playerSn);

    // 根据玩家 SN 移除玩家
    void RemovePlayerBySn(uint64 playerSn);

    // 移除所有与指定网络标识匹配的玩家
    void RemoveAllPlayers(NetIdentify* pNetIdentify);

    // 获取当前在线玩家数量
    int OnlineSize() const;

    // 获取所有玩家的指针
    std::map<uint64, Player*>* GetAll();

private:
    // 存储玩家对象的映射表，以玩家 SN 为键
    std::map<uint64, Player*> _players;
};
