#include "world_gather.h"

#include "libserver/thread_mgr.h"              // 引入线程管理器头文件
#include "libserver/network_locator.h"         // 引入网络定位器头文件
#include "libserver/message_system_help.h"     // 引入消息系统帮助类的头文件
#include "libserver/global.h"                  // 引入全局管理器头文件
#include "libserver/message_system.h"          // 引入消息系统的头文件

void WorldGather::Awake()
{
    // 每10秒同步一次空间信息，定时器的ID为2
    AddTimer(0, 10, true, 2, BindFunP0(this, &WorldGather::SyncSpaceInfo));

    // 获取消息系统
    auto pMsgSystem = GetSystemManager()->GetMessageSystem();

    // 注册处理消息函数
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_WorldSyncToGather, BindFunP1(this, &WorldGather::HandleWorldSyncToGather));
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_CmdWorld, BindFunP1(this, &WorldGather::HandleCmdWorld));
}

void WorldGather::BackToPool()
{
    // 清空地图信息
    _maps.clear();
}

void WorldGather::SyncSpaceInfo()
{
    Proto::AppInfoSync proto;  // 创建应用信息同步的协议

    int online = 0;  // 在线玩家计数
    auto iter = _maps.begin();
    while (iter != _maps.end())
    {
        online += iter->second;  // 累加在线玩家数
        ++iter;                  // 迭代器前进
    }

    const auto pGlobal = Global::GetInstance();  // 获取全局实例
    proto.set_app_id(pGlobal->GetCurAppId());    // 设置当前应用ID
    proto.set_app_type(static_cast<int>(pGlobal->GetCurAppType()));  // 设置当前应用类型
    proto.set_online(online);  // 设置在线玩家数

    // 发送同步包到所有游戏应用
    MessageSystemHelp::SendPacketToAllApp(Proto::MsgId::MI_AppInfoSync, proto, APP_GAME);

    // 同步到应用管理器
    MessageSystemHelp::SendPacket(Proto::MsgId::MI_AppInfoSync, proto, APP_APPMGR);
}

void WorldGather::HandleWorldSyncToGather(Packet* pPacket)
{
    // 解析收到的世界同步包
    auto proto = pPacket->ParseToProto<Proto::WorldSyncToGather>();
    _maps[proto.world_sn()] = proto.online();  // 更新地图中的在线玩家数
}

void WorldGather::HandleCmdWorld(Packet* pPacket)
{
    // 打印当前所有世界的在线玩家数
    LOG_DEBUG("**** world gather ****");
    for (auto one : _maps)
    {
        LOG_DEBUG("sn:" << one.first << " online:" << one.second);  // 打印世界序号和在线人数
    }
}
