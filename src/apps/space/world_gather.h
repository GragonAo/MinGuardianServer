#pragma once  // 确保头文件只被包含一次

#include "libserver/entity.h"  // 引入 Entity 基类
#include "libserver/system.h"   // 引入 System 接口

class Packet;  // 前向声明 Packet 类，避免循环依赖

// WorldGather 类负责收集和管理各个世界的在线玩家信息
class WorldGather : public Entity<WorldGather>, public IAwakeFromPoolSystem<>
{
public:
    // 初始化方法，重写自 IAwakeFromPoolSystem<>
    void Awake() override;
    
    // 清理方法，重写自 IAwakeFromPoolSystem<>
    void BackToPool() override;

private:
    // 定期同步空间信息
    void SyncSpaceInfo();
    
    // 处理来自世界的同步请求
    void HandleWorldSyncToGather(Packet* pPacket);
    
    // 处理命令世界的请求
    void HandleCmdWorld(Packet* pPacket);

private:
    // 存储地图的在线玩家数，以世界序号为键
    std::map<uint64, int> _maps;
};
