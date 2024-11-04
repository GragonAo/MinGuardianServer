#pragma once
#include "common.h"
#include "app_type.h"
#include "entity.h"

// 前向声明 Packet 类
class Packet;

// 应用程序信息结构
struct AppInfo
{
    APP_TYPE AppType; // 应用类型
    int AppId; // 应用 ID
    std::string Ip; // IP 地址
    int Port; // 端口号
    int Online; // 在线状态
    SOCKET Socket; // 套接字

    // 解析应用信息
    bool Parse(Proto::AppInfoSync proto);
};

// 同步组件类
class SyncComponent
{
public:
    // 处理应用信息同步
    void AppInfoSyncHandle(Packet* pPacket);
    
    // 显示命令
    void CmdShow();

protected:
    // 获取指定类型的应用信息
    bool GetOneApp(APP_TYPE appType, AppInfo* pInfo);
    
    // 解析应用信息
    void Parse(Proto::AppInfoSync proto, SOCKET socket);

    // 处理应用命令
    void HandleCmdApp(Packet* pPacket);
    
    // 处理网络断开连接（虚函数，可以被重写）
    virtual void HandleNetworkDisconnect(Packet* pPacket);

protected:
    // 应用 ID 和对应的应用信息映射
    std::map<int, AppInfo> _apps;
};
