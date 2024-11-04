#include "space_sync_handler.h"

#include "libserver/thread_mgr.h"
#include "libserver/message_system.h"

// 初始化方法
void SpaceSyncHandler::Awake()
{
    // 获取消息系统
    auto pMsgSystem = GetSystemManager()->GetMessageSystem();
    
    // 注册消息处理函数
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_AppInfoSync, BindFunP1(this, &SpaceSyncHandler::HandleAppInfoSync));
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_CmdApp, BindFunP1(this, &SpaceSyncHandler::HandleCmdApp));
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_NetworkDisconnect, BindFunP1(this, &SpaceSyncHandler::HandleNetworkDisconnect));
}

// 归还到对象池
void SpaceSyncHandler::BackToPool()
{
    // 清空应用信息
    _apps.clear();
}

// 获取一个当前可用的 AppId
bool SpaceSyncHandler::GetSpaceApp(AppInfo* pInfo)
{
    return GetOneApp(APP_SPACE, pInfo); // 调用获取应用的函数
}

// 处理应用信息同步的逻辑
void SpaceSyncHandler::HandleAppInfoSync(Packet* pPacket)
{
    AppInfoSyncHandle(pPacket); // 处理应用信息同步的具体逻辑
}
