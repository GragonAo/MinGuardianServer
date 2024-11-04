#include "world_proxy_help.h"

#include "libserver/message_system_help.h"
#include "libplayer/player.h"
#include "libserver/thread_mgr.h"

// 实现玩家传送功能
void WorldProxyHelp::Teleport(Player* pPlayer, const uint64 lastWorldSn, const uint64 targetWorldSn)
{
    // 获取玩家的序号
    const uint64 playerSn = pPlayer->GetPlayerSN();

    // 创建传送消息的协议对象
    Proto::Teleport proto;
    // 设置当前世界序号
    proto.set_last_world_sn(lastWorldSn);
    // 设置玩家账户信息
    proto.set_account(pPlayer->GetAccount().c_str());
    // 设置玩家序号
    proto.set_player_sn(playerSn);
    // 序列化玩家信息到协议对象中
    pPlayer->SerializeToProto(proto.mutable_player());

    // 创建网络标识对象
    NetIdentify netIdentify;
    // 复制玩家的 SocketKey
    netIdentify.GetSocketKey()->CopyFrom(pPlayer->GetSocketKey());
    // 为目标世界设置标记
    netIdentify.GetTagKey()->AddTag(TagType::Entity, targetWorldSn);
    
    // 分发传送消息
    MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_Teleport, proto, &netIdentify);
}
