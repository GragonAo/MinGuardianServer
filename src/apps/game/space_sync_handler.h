#pragma once

#include "libserver/entity.h"
#include "libserver/sync_component.h"

// 空间同步处理类
class SpaceSyncHandler : public SyncComponent, public Entity<SpaceSyncHandler>, public IAwakeSystem<>
{
public:
    // 初始化方法
    void Awake() override;
    
    // 归还到对象池
    void BackToPool() override;

    // 获取一个当前可用的 AppId
    bool GetSpaceApp(AppInfo* pInfo);

protected:
    // 处理应用信息同步的逻辑
    void HandleAppInfoSync(Packet* pPacket);
};
