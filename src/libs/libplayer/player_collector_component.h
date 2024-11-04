#pragma once
#include "libserver/system.h"
#include "libserver/component.h"
#include "libserver/socket_object.h"

class Player;

// PlayerCollectorComponent 类用于管理在线玩家的集合
class PlayerCollectorComponent : public Component<PlayerCollectorComponent>, public IAwakeFromPoolSystem<>
{
public:
    // 初始化组件
    void Awake() override;

    // 清理并返回到对象池
    void BackToPool() override;

    // 添加新玩家到集合中
    Player* AddPlayer(NetIdentify* pIdentify, std::string account);

    // 根据 SOCKET 移除玩家
    void RemovePlayerBySocket(SOCKET socket);

    // 根据玩家序列号移除玩家
    void RemovePlayerBySn(uint64 playerSn);

    // 移除所有玩家并关闭连接
    void RemoveAllPlayerAndCloseConnect();

    // 根据 SOCKET 获取玩家
    Player* GetPlayerBySocket(SOCKET socket);

    // 根据账号获取玩家
    Player* GetPlayerByAccount(std::string account);

    // 根据玩家序列号获取玩家
    Player* GetPlayerBySn(uint64 playerSn);

    // 获取在线玩家数量
    int OnlineSize() const;

    // 获取所有玩家的集合
    std::map<SOCKET, Player*>& GetAll();

protected:
    // 从集合中移除指定玩家
    void RemovePlayer(Player* pPlayer);

private:
    // 存储玩家与其 SOCKET 的映射，格式为 <socket, Player*>
    std::map<SOCKET, Player*> _players;

    // 存储账号与其 SOCKET 的映射，格式为 <account, socket>
    std::map<std::string, SOCKET> _accounts;
};
