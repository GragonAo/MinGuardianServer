#include "world_proxy_locator.h"

#include "libresource/resource_manager.h"
#include "libserver/thread_mgr.h"
#include "libserver/component_help.h"
#include "libresource/resource_help.h"
#include "libserver/message_system.h"
#include "world_proxy.h"

// 唤醒系统，进行初始化
void WorldProxyLocator::Awake()
{
    auto pMsgSystem = GetSystemManager()->GetMessageSystem();
    // 注册处理创建世界的广播消息的函数
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_BroadcastCreateWorld, BindFunP1(this, &WorldProxyLocator::HandleBroadcastCreateWorld));
}

// 回收资源，清理状态
void WorldProxyLocator::BackToPool()
{
    std::lock_guard<std::mutex> guard(_lock); // 确保线程安全
    _publics.clear(); // 清空公共世界的映射
    _worlds.clear();  // 清空世界序号的集合
}

// 注册世界到定位器
void WorldProxyLocator::RegisterToLocator(int worldId, const uint64 worldSn)
{
    std::lock_guard<std::mutex> guard(_lock); // 确保线程安全

    auto pResMgr = ResourceHelp::GetResourceManager();
    const auto mapConfig = pResMgr->Worlds->GetResource(worldId);

    // 检查是否为公共世界
    if (mapConfig->IsType(ResourceWorldType::Public))
    {
        if (_publics.find(worldId) != _publics.end())
        {
            LOG_ERROR(" WorldProxyLocator. find same key. worldId:" << worldId); // 错误：重复的世界ID
        }

        _publics[worldId] = worldSn; // 注册公共世界
    }

    _worlds.emplace(worldSn); // 添加世界序号
}

// 移除已注册的世界
void WorldProxyLocator::Remove(const int worldId, const uint64 worldSn)
{
    std::lock_guard<std::mutex> guard(_lock); // 确保线程安全
    _publics.erase(worldId); // 移除公共世界
    _worlds.erase(worldSn);   // 移除世界序号
}

// 检查指定的世界序号是否存在于地下城中
bool WorldProxyLocator::IsExistDungeon(const uint64 mspSn)
{
    std::lock_guard<std::mutex> guard(_lock); // 确保线程安全
    return _worlds.find(mspSn) != _worlds.end(); // 返回是否存在
}

// 根据世界ID获取对应的世界序号
uint64 WorldProxyLocator::GetWorldSnById(const int worldId)
{
    std::lock_guard<std::mutex> guard(_lock); // 确保线程安全
    const auto iter = _publics.find(worldId);
    
    if (iter == _publics.end())
        return INVALID_ID; // 找不到时返回无效ID

    return iter->second; // 返回找到的世界序号
}

// 处理创建世界的广播消息
void WorldProxyLocator::HandleBroadcastCreateWorld(Packet* pPacket)
{
    std::lock_guard<std::mutex> guard(_lock); // 确保线程安全
    auto proto = pPacket->ParseToProto<Proto::BroadcastCreateWorld>(); // 解析消息
    const int worldId = proto.world_id();
    const auto worldSn = proto.world_sn();
    const uint64 lastWorldSn = proto.last_world_sn();

    const auto worldCfg = ResourceHelp::GetResourceManager()->Worlds->GetResource(worldId);
    if (worldCfg == nullptr)
    {
        LOG_ERROR("WorldProxyLocator::HandleBroadcastCreateWorld. can't find worldId:" << worldId);
        return; // 如果未找到世界配置，记录错误
    }

    // 处理公共世界
    if (worldCfg->IsType(ResourceWorldType::Public))
    {
        if (_publics.find(worldId) != _publics.end())
        {
            LOG_ERROR(" WorldLocator. find same key. worldId:" << worldId);
            return; // 如果已存在相同的世界ID，记录错误
        }

        ThreadMgr::GetInstance()->CreateComponentWithSn<WorldProxy>(worldSn, worldId, lastWorldSn);
    }
    else // 处理地下城
    {
        if (_worlds.find(lastWorldSn) == _worlds.end())
        {
            LOG_ERROR("can't find request world. world sn:" << lastWorldSn);
            return; // 如果请求的世界序号不存在，记录错误
        }

        ThreadMgr::GetInstance()->CreateComponentWithSn<WorldProxy>(worldSn, worldId, lastWorldSn);
    }
}
