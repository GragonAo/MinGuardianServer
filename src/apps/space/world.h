#pragma once
#include "libserver/system.h" // 引入服务器系统相关的头文件
#include "libplayer/world_base.h" // 引入玩家世界基础类
#include "libserver/entity.h" // 引入实体类
#include "libserver/socket_object.h" // 引入套接字对象类

class Player; // 前向声明 Player 类

// World 类定义，继承自 Entity 和 IWorld 接口，同时实现 IAwakeFromPoolSystem
class World : public Entity<World>, public IWorld, public IAwakeFromPoolSystem<int>
{
public:
    // 初始化世界，传入世界 ID
    void Awake(int worldId) override;

    // 将世界返回到池中，释放资源
    void BackToPool() override;

protected:
    // 根据网络标识获取对应的玩家
    Player* GetPlayer(NetIdentify* pIdentify);

    // 广播消息给所有玩家
    void BroadcastPacket(Proto::MsgId msgId, google::protobuf::Message& proto);

    // 广播消息给指定玩家集合
    void BroadcastPacket(Proto::MsgId msgId, google::protobuf::Message& proto, std::set<uint64> players);

    // 处理网络断开连接的情况
    void HandleNetworkDisconnect(Packet* pPacket);

    // 处理同步玩家信息的消息
    void HandleSyncPlayer(Packet* pPacket);

    // 处理请求同步玩家信息的消息
    void HandleRequestSyncPlayer(Player* pPlayer, Packet* pPacket);

    // 处理客户端请求移除玩家的消息
    void HandleG2SRemovePlayer(Player* pPlayer, Packet* pPacket);

    // 处理玩家移动的消息
    void HandleMove(Player* pPlayer, Packet* pPacket);

private:
    // 同步世界状态到收集器
    void SyncWorldToGather();

    // 同步玩家外观定时器
    void SyncAppearTimer();

private:
    // 存储添加的新玩家的序列号，用于管理玩家的出现与消失
    std::set<uint64> _addPlayer;
};
