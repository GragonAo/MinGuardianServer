#pragma once
#include "libserver/component.h"
#include "libserver/system.h"

// 传送状态标志
// 1. 传送需要等待目标世界的资源准备
// 2. 传送需要等待相关的数据同步与实际世界的一致性

enum class TeleportFlagType
{
    None = 0,        // 未设置状态
    Waiting = 1,     // 等待状态
    Completed = 2,   // 完成状态
};

// 传送标志模板结构
template<typename T>
struct TeleportFlag
{
public:
    friend class TeleportObject; // 允许 TeleportObject 访问私有成员

    TeleportFlagType Flag; // 当前状态标志

    // 设置值并标记为完成
    void SetValue(T value)
    {
        this->Value = value;
        this->Flag = TeleportFlagType::Completed; // 设置状态为完成
    }

    // 获取当前值
    T GetValue()
    {
        return this->Value;
    }

    // 判断传送是否完成
    bool IsCompleted()
    {
        return this->Flag == TeleportFlagType::Completed;
    }

private:
    T Value; // 存储的值
};

// 传送对象类
class TeleportObject : public Component<TeleportObject>, public IAwakeFromPoolSystem<int, uint64>
{
public:
    // 初始化方法，传入目标世界 ID 和玩家序号
    void Awake(int worldId, uint64 playerSn) override;
    
    // 归还到对象池
    void BackToPool() override;

    // 传送标志
    TeleportFlag<uint64> FlagWorld;     // 目标世界的传送标志
    TeleportFlag<bool> FlagPlayerSync;   // 玩家同步的传送标志

    // 获取目标世界 ID
    int GetTargetWorldId() const;
    
    // 获取玩家序号
    uint64 GetPlayerSN() const;

private:
    int _targetWorldId{ 0 }; // 目标世界 ID
    uint64 _playerSn{ 0 };   // 玩家序号
};
