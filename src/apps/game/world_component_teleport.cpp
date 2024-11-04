#include "world_component_teleport.h"
#include "world_proxy_locator.h"
#include "world_proxy_help.h"
#include "teleport_object.h"
#include "world_proxy.h"

#include "libserver/message_system_help.h"
#include "libserver/sync_component.h"

#include "libresource/resource_world.h"
#include "libresource/resource_manager.h"
#include "libresource/resource_help.h"

#include "libplayer/player.h"
#include "libplayer/player_collector_component.h"
#include "space_sync_handler.h"
#include "libserver/component_help.h"

// 初始化方法
void WorldComponentTeleport::Awake()
{
}

// 归还到对象池的方法
void WorldComponentTeleport::BackToPool()
{
    if (!_objects.empty())
    {
        LOG_WARN("not completed to teleport. WorldComponentTeleport be destroyed.");
    }

    for (const auto pair : _objects)
    {
        GetSystemManager()->GetEntitySystem()->RemoveComponent(pair.second);
    }
    _objects.clear(); // 清空对象集合
}

// 检查玩家是否正在传送
bool WorldComponentTeleport::IsTeleporting(Player* pPlayer)
{
    return _objects.find(pPlayer->GetPlayerSN()) != _objects.end();
}

// 创建传送对象
void WorldComponentTeleport::CreateTeleportObject(int worldId, Player* pPlayer)
{
    const auto pObj = GetSystemManager()->GetEntitySystem()->AddComponent<TeleportObject>(worldId, pPlayer->GetPlayerSN());
    _objects.insert(std::make_pair(pPlayer->GetPlayerSN(), pObj));

    const auto pWorldProxy = GetParent<WorldProxy>();

    // 创建世界标志
    CreateWorldFlag(pWorldProxy, worldId, pObj);

    // 创建同步标志
    CreateSyncFlag(pWorldProxy, pObj);
}

// 创建世界标志，处理世界的不同类型
void WorldComponentTeleport::CreateWorldFlag(WorldProxy* pWorldProxy, int targetWorldId, TeleportObject* pObj)
{
    const auto pResMgr = ResourceHelp::GetResourceManager();
    const auto pWorldRes = pResMgr->Worlds->GetResource(targetWorldId);

    if (pWorldRes->IsType(ResourceWorldType::Public))
    {
        auto pWorldLocator = ComponentHelp::GetGlobalEntitySystem()->GetComponent<WorldProxyLocator>();
        const auto worldSn = pWorldLocator->GetWorldSnById(targetWorldId);
        if (worldSn == static_cast<uint64>(INVALID_ID))
        {
            // 请求创建公共世界
            Proto::RequestWorld protoToMgr;
            protoToMgr.set_world_id(targetWorldId);
            MessageSystemHelp::SendPacket(Proto::MsgId::G2M_RequestWorld, protoToMgr, APP_APPMGR);

            pObj->FlagWorld.Flag = TeleportFlagType::Waiting; // 设置等待状态
        }
        else
        {
            pObj->FlagWorld.SetValue(worldSn); // 设置世界序号
        }
    }
    else if (pWorldRes->IsType(ResourceWorldType::Dungeon))
    {
        // 请求创建地下城世界
        auto pSpaceSyncHandler = ComponentHelp::GetGlobalEntitySystem()->GetComponent<SpaceSyncHandler>();
        AppInfo info;
        if (!pSpaceSyncHandler->GetSpaceApp(&info))
        {
            LOG_ERROR("can't find space");
            return;
        }

        Proto::CreateWorld protoCreate;
        protoCreate.set_world_id(targetWorldId);
        protoCreate.set_last_world_sn(pWorldProxy->GetSN());
        protoCreate.set_game_app_id(Global::GetInstance()->GetCurAppId());
        MessageSystemHelp::SendPacket(Proto::MsgId::G2S_CreateWorld, protoCreate, APP_SPACE, info.AppId);

        pObj->FlagWorld.Flag = TeleportFlagType::Waiting; // 设置等待状态
    }
    else
    {
        LOG_ERROR("WorldComponentTeleport");
    }
}

// 创建同步标志
void WorldComponentTeleport::CreateSyncFlag(WorldProxy* pWorldProxy, TeleportObject* pObj)
{
    auto pPlayerMgr = _parent->GetComponent<PlayerCollectorComponent>();
    const auto pPlayer = pPlayerMgr->GetPlayerBySn(pObj->GetPlayerSN());

    Proto::RequestSyncPlayer protoSync;
    protoSync.set_player_sn(pObj->GetPlayerSN());
    pWorldProxy->SendPacketToWorld(Proto::MsgId::G2S_RequestSyncPlayer, protoSync, pPlayer);

    pObj->FlagPlayerSync.Flag = TeleportFlagType::Waiting; // 设置等待状态
}

// 处理世界代理的创建广播
void WorldComponentTeleport::HandleBroadcastCreateWorldProxy(const int worldId, const uint64 worldSn)
{
    const auto pResMgr = ResourceHelp::GetResourceManager();
    const auto pWorldRes = pResMgr->Worlds->GetResource(worldId);
    if (pWorldRes->IsType(ResourceWorldType::Public))
    {
        // 对于公共世界
        do
        {
            auto iter = std::find_if(_objects.begin(), _objects.end(), [&worldId](auto pair)
            {
                return (pair.second->GetTargetWorldId() == worldId);
            });

            if (iter == _objects.end())
                break;

            auto pObj = iter->second;
            pObj->FlagWorld.SetValue(worldSn); // 设置世界序号
            Check(pObj); // 检查是否可以传送

        } while (true);
    }
    else if (pWorldRes->IsType(ResourceWorldType::Dungeon))
    {
        // 对于地下城
        auto iter = std::find_if(_objects.begin(), _objects.end(), [&worldId](auto pair)
        {
            return (pair.second->GetTargetWorldId() == worldId);
        });

        if (iter == _objects.end())
        {
            LOG_ERROR("BroadcastCreateWorldProxy, can't find teleport object. create world id:" << worldId << " cur world id:" << GetParent<WorldProxy>()->GetWorldId());
            return;
        }

        auto pObj = iter->second;
        pObj->FlagWorld.SetValue(worldSn); // 设置世界序号
        Check(pObj); // 检查是否可以传送
    }
}

// 广播玩家同步
void WorldComponentTeleport::BroadcastSyncPlayer(uint64 playerSn)
{
    auto iter = std::find_if(_objects.begin(), _objects.end(), [&playerSn](auto pair)
    {
        return pair.second->GetPlayerSN() == playerSn;
    });

    if (iter == _objects.end())
        return;

    auto pObj = iter->second;
    pObj->FlagPlayerSync.SetValue(true); // 设置同步完成
    Check(pObj); // 检查是否可以传送
}

// 检查传送条件
bool WorldComponentTeleport::Check(TeleportObject* pObj)
{
    const auto worldId = pObj->GetTargetWorldId();
    const auto pWorldProxy = GetParent<WorldProxy>();

    // 检查传送是否完成
    if (!pObj->FlagPlayerSync.IsCompleted() || !pObj->FlagWorld.IsCompleted())
        return false;

    auto pPlayerMgr = _parent->GetComponent<PlayerCollectorComponent>();
    const auto pPlayer = pPlayerMgr->GetPlayerBySn(pObj->GetPlayerSN());

    if (pObj->FlagWorld.GetValue() == 0)
    {
        LOG_ERROR("error. can't find target world proxy's obj. world id:" << worldId);
        return false;
    }

    // 执行传送
    WorldProxyHelp::Teleport(pPlayer, pWorldProxy->GetSN(), pObj->FlagWorld.GetValue());

    // 清理状态
    _objects.erase(pPlayer->GetPlayerSN());
    GetSystemManager()->GetEntitySystem()->RemoveComponent(pObj);
    return true; // 传送成功
}
