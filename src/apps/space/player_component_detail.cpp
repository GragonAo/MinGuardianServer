#include "player_component_detail.h"            // 包含 PlayerComponentDetail 的头文件
#include "libplayer/player.h"                    // 包含 Player 类的头文件
#include "libserver/message_system_help.h"       // 包含消息系统帮助的头文件

// Awake 方法在组件被激活时调用
void PlayerComponentDetail::Awake()
{
    // 动态转换 _parent 成员为 Player 类型
    Player* pPlayer = dynamic_cast<Player*>(_parent);
    // 从 Player 对象中获取 Proto 数据并解析
    ParserFromProto(pPlayer->GetPlayerProto());
}

// BackToPool 方法在组件被返回到对象池时调用
void PlayerComponentDetail::BackToPool()
{
    // 目前没有需要清理的操作，可以在这里实现
}

// 从 Proto 对象解析数据
void PlayerComponentDetail::ParserFromProto(const Proto::Player& proto)
{
    // 获取 Proto 对象的基本信息
    auto protoBase = proto.base();
    // 设置玩家的性别
    _gender = protoBase.gender();
}

// 将组件的数据序列化到 Proto 对象中
void PlayerComponentDetail::SerializeToProto(Proto::Player* pProto)
{
    // 该方法目前没有实现序列化逻辑
}

// 获取玩家的性别
Proto::Gender PlayerComponentDetail::GetGender() const
{
    return _gender; // 返回存储的性别
}
