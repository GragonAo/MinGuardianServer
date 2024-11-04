#include "world_operator_component.h"  // 引入头文件
#include "world.h"  // 引入 World 类

#include "libserver/thread_mgr.h"  // 引入线程管理器
#include "libserver/message_system.h"  // 引入消息系统

#include "libresource/resource_manager.h"  // 引入资源管理器
#include "libresource/resource_help.h"  // 引入资源帮助函数

// 初始化方法，注册消息处理函数
void WorldOperatorComponent::Awake()
{
    auto pMsgSystem = GetSystemManager()->GetMessageSystem();  // 获取消息系统
    pMsgSystem->RegisterFunction(this, Proto::MsgId::G2S_CreateWorld, BindFunP1(this, &WorldOperatorComponent::HandleCreateWorld));  // 注册创建世界的消息处理
}

// 清理方法
void WorldOperatorComponent::BackToPool()
{
    // 这里可以添加清理逻辑
}

// 处理创建世界的消息
void WorldOperatorComponent::HandleCreateWorld(Packet* pPacket)
{
    // 解析消息数据
    auto protoWorld = pPacket->ParseToProto<Proto::CreateWorld>();
    int worldId = protoWorld.world_id();  // 获取世界ID
    const int gameAppId = protoWorld.game_app_id();  // 获取游戏应用ID
    const uint64 lastWorldSn = protoWorld.last_world_sn();  // 获取上一个世界的序号

    auto worldSn = Global::GetInstance()->GenerateSN();  // 生成新的世界序号
    ThreadMgr::GetInstance()->CreateComponentWithSn<World>(worldSn, worldId);  // 创建世界组件

    // 记录日志
    //LOG_DEBUG("create world. map id:" << worldId << " world sn:" << newWorldSn);
 
    const auto pResMgr = ResourceHelp::GetResourceManager();  // 获取资源管理器
    const auto pWorldRes = pResMgr->Worlds->GetResource(worldId);  // 获取指定世界的资源
    if (pWorldRes->IsType(ResourceWorldType::Dungeon) && lastWorldSn == 0)
    {
        LOG_ERROR("create world error. dungeon is created. but requestWorldSn == 0");  // 错误处理
    }

    // 构建广播消息
    Proto::BroadcastCreateWorld protoRs;
    protoRs.set_world_id(worldId);
    protoRs.set_world_sn(worldSn);
    protoRs.set_last_world_sn(lastWorldSn);

    // 向应用管理器发送创建世界的消息
    if ((Global::GetInstance()->GetCurAppType() & APP_APPMGR) == 0)
    {
        // 如果当前应用不是应用管理器，向应用管理器发送消息
        MessageSystemHelp::SendPacket(Proto::MsgId::MI_BroadcastCreateWorld, protoRs, APP_APPMGR);
    }

    // 向游戏应用发送创建世界的消息
    if ((Global::GetInstance()->GetCurAppType() & APP_GAME) == 0)
    {
        if (gameAppId != 0)
        {
            // 如果指定了游戏应用ID，发送到该游戏应用
            MessageSystemHelp::SendPacket(Proto::MsgId::MI_BroadcastCreateWorld, protoRs, APP_GAME, gameAppId);
        }
        else
        {
            // 否则，广播到所有游戏应用
            MessageSystemHelp::SendPacketToAllApp(Proto::MsgId::MI_BroadcastCreateWorld, protoRs, APP_GAME);
        }
    }

    // 如果当前应用是游戏或应用管理器，分发创建世界消息
    if ((Global::GetInstance()->GetCurAppType() & APP_GAME) != 0 || (Global::GetInstance()->GetCurAppType() & APP_APPMGR) != 0)
    {
        MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_BroadcastCreateWorld, protoRs, nullptr);
    }
}
