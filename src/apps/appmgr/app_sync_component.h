#pragma once

#include "libserver/sync_component.h"
#include "libserver/system.h"
#include <json/writer.h>

class AppSyncComponent : public Entity<AppSyncComponent>, public SyncComponent, public IAwakeSystem<>
{
public:
    // 初始化组件
    void Awake() override;

    // 清理资源并将组件返回到池中
    void BackToPool() override;

protected:
    // 处理登录的 HTTP 请求
    void HandleHttpRequestLogin(Packet* pPacket);

    // 处理应用信息同步的消息
    void HandleAppInfoSync(Packet* pPacket);

    // 处理网络断开连接的逻辑
    void HandleNetworkDisconnect(Packet* pPacket) override;

private:
    // 将游戏信息同步到登录服务
    void SyncGameInfoToLogin();

private:
    Json::StreamWriter* _jsonWriter{ nullptr }; // 用于 JSON 格式化的写入器
};
