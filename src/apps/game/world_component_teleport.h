#pragma once
#include "libserver/component.h"
#include "libserver/system.h"

class WorldProxy;      // 前向声明，表示世界代理
class Player;         // 前向声明，表示玩家
class ResourceWorld;  // 前向声明，表示资源世界
class TeleportObject; // 前向声明，表示传送对象

class WorldComponentTeleport : public Component<WorldComponentTeleport>, public IAwakeFromPoolSystem<>
{
public:
    void Awake() override; // 初始化方法
    void BackToPool() override; // 归还对象到池中的方法

    bool IsTeleporting(Player* pPlayer); // 检查玩家是否正在传送

    void CreateTeleportObject(int worldId, Player* pPlayer); // 创建传送对象
    void HandleBroadcastCreateWorldProxy(int worldId, uint64 worldSn); // 处理创建世界代理的广播
    void BroadcastSyncPlayer(uint64 playerSn); // 广播玩家同步

protected:
    void CreateWorldFlag(WorldProxy* pWorldProxy, int targetWorldId, TeleportObject* pObj); // 创建世界标志
    void CreateSyncFlag(WorldProxy* pWorldProxy, TeleportObject* pObj); // 创建同步标志

    bool Check(TeleportObject* pObj); // 检查传送条件

private:
    std::map<uint64, TeleportObject*> _objects; // 存储正在传送的对象，键为玩家序号，值为传送对象指针
};
