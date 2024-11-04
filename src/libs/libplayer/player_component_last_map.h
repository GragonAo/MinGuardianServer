#pragma once
#include "libserver/component.h"
#include "player_component.h"
#include "libserver/vector3.h"
#include "libserver/system.h"

// 记录玩家最后所在的世界信息
struct LastWorld
{
    int WorldId{ 0 };           // 世界 ID
    uint64 WorldSn{ 0 };        // 世界序列号
    Vector3 Position{ 0, 0, 0 }; // 玩家在世界中的位置

    // 构造函数，使用世界 ID、序列号和位置初始化
    LastWorld(const int worldId, const uint64 worldSn, const Vector3 pos);

    // 从 Proto 对象构造
    LastWorld(Proto::LastWorld proto);

    // 将数据序列化到 Proto 对象中
    void SerializeToProto(Proto::LastWorld* pProto) const;
};

// 玩家组件类，记录玩家最后的地图信息
class PlayerComponentLastMap : public Component<PlayerComponentLastMap>,
                                public IAwakeFromPoolSystem<>,
                                public PlayerComponent
{
public:
    // 初始化玩家组件
    void Awake() override;

    // 清理并返回到对象池
    void BackToPool() override;

    // 从 Proto 对象解析数据
    void ParserFromProto(const Proto::Player& proto) override;

    // 序列化数据到 Proto 对象
    void SerializeToProto(Proto::Player* pProto) override;

    // 获取最近的公共地图
    LastWorld* GetLastPublicMap() const;

    // 获取最近的地下城
    LastWorld* GetLastDungeon() const;

    // 获取当前世界信息
    LastWorld* GetCur() const;

    // 进入新的世界
    void EnterWorld(int worldId, uint64 worldSn);

    // 设置当前世界 ID
    void SetCurWorld(int worldId);

protected:
    // 进入地下城
    void EnterDungeon(int worldId, uint64 worldSn, Vector3 position);    

private:
    // 最近的公共地图和地下城
    LastWorld* _pPublic{ nullptr }; // 最近的公共世界
    LastWorld* _pDungeon{ nullptr }; // 最近的地下城

    int _curWorldId{ 0 }; // 当前世界 ID
};
