#pragma once
#include "network.h"
#include "system.h"
#include "app_type.h"
#include "socket_object.h"

// 网络定位器类
class NetworkLocator : public Entity<NetworkLocator>, public IAwakeSystem<>
{
public:
    void Awake() override; // 初始化
    void BackToPool() override; // 返回池中

    // 连接管理
    void AddConnectorLocator(INetwork* pNetwork, NetworkType networkType); // 添加连接器定位
    void AddNetworkIdentify(SocketKey* pSocket, uint64 appKey); // 添加网络标识
    std::list<NetIdentify> GetAppNetworks(const APP_TYPE appType); // 获取指定应用类型的网络

    NetIdentify GetNetworkIdentify(const APP_TYPE appType, const int appId); // 获取指定应用的网络标识

    // 监听管理
    void AddListenLocator(INetwork* pNetwork, NetworkType networkType); // 添加监听器定位

    // 获取网络
    INetwork* GetNetwork(NetworkType networkType); // 根据网络类型获取网络接口

protected:
    void HandleAppRegister(Packet* pPacket); // 处理应用注册
    void HandleNetworkDisconnect(Packet* pPacket); // 处理网络断开连接

private:
    std::mutex _lock; // 互斥锁，保护资源访问

    // <apptype + appId, NetIdentify>
    std::map<uint64, NetIdentify> _netIdentify; // 存储应用类型和ID对应的网络标识

    // 连接器
    std::map<NetworkType, INetwork*> _connectors; // 存储连接器

    // 监听器
    std::map<NetworkType, INetwork*> _listens; // 存储监听器，TCP与HTTP同时支持
};
