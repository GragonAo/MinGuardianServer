#include "teleport_object.h"

// 初始化方法，接收目标世界 ID 和玩家序号
void TeleportObject::Awake(const int worldId, const uint64 playerSn)
{
    // 初始化目标世界的传送标志
	FlagWorld.Flag = TeleportFlagType::None; // 设置为未设置状态
	FlagWorld.Value = 0; // 初始值为0

    // 初始化玩家同步的传送标志
	FlagPlayerSync.Flag = TeleportFlagType::None; // 设置为未设置状态
	FlagPlayerSync.Value = false; // 初始值为 false

    // 设置目标世界 ID 和玩家序号
	_targetWorldId = worldId;
	_playerSn = playerSn;
}

// 归还到对象池的方法，重置所有成员变量
void TeleportObject::BackToPool()
{
    // 重置目标世界的传送标志
	FlagWorld.Flag = TeleportFlagType::None; // 设置为未设置状态
	FlagWorld.Value = 0; // 重置值为0

    // 重置玩家同步的传送标志
	FlagPlayerSync.Flag = TeleportFlagType::None; // 设置为未设置状态
	FlagPlayerSync.Value = false; // 重置值为 false

    // 重置目标世界 ID 和玩家序号
	_targetWorldId = 0; // 重置为0
	_playerSn = 0; // 重置为0
}

// 获取目标世界 ID
int TeleportObject::GetTargetWorldId() const
{
	return _targetWorldId; // 返回当前目标世界 ID
}

// 获取玩家序号
uint64 TeleportObject::GetPlayerSN() const
{
	return _playerSn; // 返回当前玩家序号
}
