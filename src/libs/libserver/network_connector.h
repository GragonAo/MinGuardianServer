#pragma once

#include <utility>
#include "network.h"
#include "connect_obj.h"
#include "app_type.h"
#include "socket_object.h"

class Packet;

// 连接详细信息结构体
struct ConnectDetail : public SnObject, public IDisposable
{
public:
    ConnectDetail(TagType tagType, TagValue tagValue, std::string ip, int port)
    {
        TType = tagType;      // 设置标签类型
        TValue = tagValue;    // 设置标签值
        Ip = std::move(ip);   // 移动IP字符串
        Port = port;          // 设置端口
    };

    void Dispose() override { } // 释放资源（空实现）

    std::string Ip{ "" }; // IP地址
    int Port{ 0 };        // 端口号

    TagType TType;        // 标签类型
    TagValue TValue;      // 标签值
};

// 网络连接器类
class NetworkConnector :
    public Network,
    public IAwakeSystem<int, int>
{
public:
    // 初始化连接器
    void Awake(int iType, int mixConnectAppType) override;

    // 更新连接器状态
    virtual void Update();

    // 获取连接器类型名称
    const char* GetTypeName() override;
    // 获取连接器类型哈希值
    uint64 GetTypeHashCode() override;
    // 发起连接
    bool Connect(ConnectDetail* pDetail);

private:
    void HandleNetworkConnect(Packet* pPacket);  // 处理网络连接的包
    void CreateConnector(APP_TYPE appType, int appId, std::string ip, int port); // 创建连接器

private:
    // 等待连接的详细信息
    CacheRefresh<ConnectDetail> _connecting;
};
