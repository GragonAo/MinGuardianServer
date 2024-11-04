#include "player_component_last_map.h"
#include "libresource/resource_manager.h"
#include "player.h"
#include "libresource/resource_help.h"
#include "libserver/vector3.h"

// LastWorld 构造函数，接受世界 ID、世界序列号和位置
LastWorld::LastWorld(const int worldId, const uint64 worldSn, const Vector3 pos)
{
    this->WorldId = worldId;
    this->WorldSn = worldSn;
    this->Position = pos;

    // LOG_DEBUG("Init. LastWorld. id:" << this->WorldId << " sn:" << this->WorldSn << this->Position);
}

// 从 Proto 对象构造 LastWorld
LastWorld::LastWorld(Proto::LastWorld proto)
{
    this->WorldId = proto.world_id();
    this->WorldSn = proto.world_sn();
    this->Position.ParserFromProto(proto.position());

    // LOG_DEBUG("ParserFromProto. LastWorld. id:" << this->WorldId << " sn:" << this->WorldSn << this->Position);
}

// 将 LastWorld 序列化到 Proto 对象
void LastWorld::SerializeToProto(Proto::LastWorld* pProto) const
{
    pProto->set_world_id(WorldId);
    pProto->set_world_sn(WorldSn);
    Position.SerializeToProto(pProto->mutable_position());

    // LOG_DEBUG("SerializeToProto. LastWorld. id:" << this->WorldId << " sn:" << this->WorldSn << this->Position);
}

// PlayerComponentLastMap 的初始化
void PlayerComponentLastMap::Awake()
{
    Player* pPlayer = dynamic_cast<Player*>(_parent);
    ParserFromProto(pPlayer->GetPlayerProto());
}

// 从 Proto 对象解析数据
void PlayerComponentLastMap::ParserFromProto(const Proto::Player& proto)
{
    // 解析公共地图
    auto protoMap = proto.misc().last_world();
    int worldId = protoMap.world_id();
    auto pResMgr = ResourceHelp::GetResourceManager();
    auto pMap = pResMgr->Worlds->GetResource(worldId);
    
    if (pMap != nullptr)
    {
        _pPublic = new LastWorld(protoMap);
    }
    else
    {
        pMap = pResMgr->Worlds->GetInitMap();
        _pPublic = new LastWorld(pMap->GetId(), 0, pMap->GetInitPosition());
    }

    // 解析地下城
    auto protoDungeon = proto.misc().last_dungeon();
    worldId = protoDungeon.world_id();
    pMap = pResMgr->Worlds->GetResource(worldId);
    
    if (pMap != nullptr)
    {
        _pDungeon = new LastWorld(protoDungeon);
    }
}

// 将数据序列化到 Proto 对象
void PlayerComponentLastMap::SerializeToProto(Proto::Player* pProto)
{
    if (_pPublic != nullptr)
    {
        const auto pLastMap = pProto->mutable_misc()->mutable_last_world();
        _pPublic->SerializeToProto(pLastMap);
    }

    if (_pDungeon != nullptr)
    {
        const auto pLastDungeon = pProto->mutable_misc()->mutable_last_dungeon();
        _pDungeon->SerializeToProto(pLastDungeon);
    }
}

// 获取上一个公共地图
LastWorld* PlayerComponentLastMap::GetLastPublicMap() const
{
    return _pPublic;
}

// 获取上一个地下城
LastWorld* PlayerComponentLastMap::GetLastDungeon() const
{
    return _pDungeon;
}

// 获取当前所在的地图
LastWorld* PlayerComponentLastMap::GetCur() const
{
    if (_pPublic->WorldId == _curWorldId)
        return _pPublic;

    return _pDungeon;
}

// 进入新的世界
void PlayerComponentLastMap::EnterWorld(const int worldId, const uint64 worldSn)
{
    auto pResMgr = ResourceHelp::GetResourceManager();
    const auto pWorldRes = pResMgr->Worlds->GetResource(worldId);
    _curWorldId = worldId;
    
    if (pWorldRes->IsType(ResourceWorldType::Public))
    {
        auto pLastMap = GetLastPublicMap();
        if (pLastMap->WorldId == worldId)
        {
            // 更新当前的序列号
            pLastMap->WorldId = worldId;
            pLastMap->WorldSn = worldSn;
        }
        else
        {
            pLastMap->WorldId = worldId;
            pLastMap->WorldSn = worldSn;
            pLastMap->Position = pWorldRes->GetInitPosition();
        }
    }
    else
    {
        EnterDungeon(worldId, worldSn, pWorldRes->GetInitPosition());
    }
}

// 设置当前世界 ID
void PlayerComponentLastMap::SetCurWorld(int worldId)
{
    _curWorldId = worldId;
}

// 进入地下城
void PlayerComponentLastMap::EnterDungeon(const int worldId, const uint64 worldSn, const Vector3 position)
{
    if (_pDungeon == nullptr)
    {
        _pDungeon = new LastWorld(worldId, worldSn, position);
    }

    if (_pDungeon->WorldSn != worldSn)
    {
        _pDungeon->WorldId = worldId;
        _pDungeon->WorldSn = worldSn;
        _pDungeon->Position = position;
    }
}

// 清理并返回到对象池
void PlayerComponentLastMap::BackToPool()
{
    if (_pPublic != nullptr)
    {
        delete _pPublic;
        _pPublic = nullptr;
    }

    if (_pDungeon != nullptr)
    {
        delete _pDungeon;
        _pDungeon = nullptr;
    }
}
