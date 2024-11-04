#include "world_proxy_component_gather.h"

#include "libserver/message_system_help.h"
#include "libplayer/player_collector_component.h"
#include "libplayer/world_base.h"

void WorldProxyComponentGather::Awake()
{
    const auto pWorld = GetParent(); // 获取父组件（WorldProxy）
    _worldSn = pWorld->GetSN(); // 获取世界序号
    _worldId = dynamic_cast<IWorld*>(_parent)->GetWorldId(); // 获取世界ID
    AddTimer(0, 2, false, 0, BindFunP0(this, &WorldProxyComponentGather::SyncWorldInfoToGather)); // 每2秒同步一次世界信息
}

void WorldProxyComponentGather::BackToPool()
{
    Proto::WorldProxySyncToGather proto;
    proto.set_world_sn(_worldSn);
    proto.set_is_remove(true); // 标记为移除

    MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_WorldProxySyncToGather, proto, nullptr); // 发送移除信息
}

void WorldProxyComponentGather::SyncWorldInfoToGather()
{
    Proto::WorldProxySyncToGather proto;
    proto.set_world_sn(_worldSn);
    proto.set_is_remove(false); // 标记为不移除
    proto.set_world_id(_worldId); // 设置世界ID

    const int online = _parent->GetComponent<PlayerCollectorComponent>()->OnlineSize(); // 获取在线玩家数量
    proto.set_online(online); // 设置在线人数

    MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_WorldProxySyncToGather, proto, nullptr); // 发送同步信息
}
