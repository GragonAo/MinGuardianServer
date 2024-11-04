#pragma once
#include "system.h"
#include "entity.h"

class Packet;

class SocketLocator : public Entity<SocketLocator>, public IAwakeSystem<>
{
public:
    void Awake() override; // 初始化方法
    void BackToPool() override; // 返回对象池的方法

    void RegisterToLocator(SOCKET socket, uint64 sn); // 注册套接字到定位器
    void Remove(SOCKET socket); // 从定位器中移除套接字
    uint64 GetTargetEntitySn(SOCKET socket); // 获取目标实体的序列号

private:
    void HandleNetworkDisconnect(Packet* pPacket); // 处理网络断开的方法

private:
    std::mutex _lock; // 用于线程安全的锁
    std::map<SOCKET, uint64> _componentes; // 存储套接字与实体序列号的映射
};
