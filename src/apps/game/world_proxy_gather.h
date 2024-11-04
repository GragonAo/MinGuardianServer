#pragma once

#include "libserver/system.h"
#include "libserver/entity.h"

class Packet;

struct WorldProxyInfo
{
	uint64 WorldSn; // 世界序号
	int WorldId;    // 世界ID
	int Online;     // 在线玩家数量
};

class WorldProxyGather : public Entity<WorldProxyGather>, public IAwakeSystem<>
{
public:
	void Awake() override; // 初始化方法
    void BackToPool() override; // 释放资源的方法

private:
	void SyncGameInfo(); // 同步游戏信息的方法
	void HandleWorldProxySyncToGather(Packet* pPacket); // 处理同步信息的方法
	void HandleCmdWorldProxy(Packet* pPacket); // 处理命令的方法

private:
	std::map<uint64, WorldProxyInfo> _maps; // 存储世界代理信息的映射表
};


/*
WorldProxyGather 类负责管理游戏中世界代理的信息，
能够同步当前世界的状态并处理相关命令。
这为游戏的多世界架构提供了必要的信息流动和管理能力。
*/