#pragma once

#include "libserver/socket_object.h"
#include "object_pool.h"     // 包含对象池的定义
#include "packet.h"          // 包含 Packet 类的定义
#include "singleton.h"       // 包含单例模式的定义

#include <mutex>             // 引入互斥锁，保证多线程安全

/**
 * @class DynamicPacketPool
 * @brief 动态分配 Packet 对象的内存池，继承自 DynamicObjectPool 和 Singleton。
 *        使用互斥锁保护 Packet 的分配和释放操作，确保线程安全。
 */
class DynamicPacketPool : public DynamicObjectPool<Packet>, public Singleton<DynamicPacketPool>
{
public:
    /**
     * @brief 从对象池中分配一个 Packet 对象，并初始化消息 ID 和网络标识。
     * 
     * @param msgId 消息 ID，用于标识当前 Packet 对象的消息类型。
     * @param pIdentify 指向 NetworkIdentify 的指针，用于标识网络连接的唯一标识。
     * @return Packet* 返回分配的 Packet 对象指针。
     */
    Packet* MallocPacket(Proto::MsgId msgId, NetIdentify* pIdentify);

    /**
     * @brief 定期更新对象池的状态，可能用于处理超时对象或清理操作。
     *        该函数会覆盖基类中的 Update 方法。
     */
    virtual void Update() override;

    /**
     * @brief 释放对象到对象池中，并且覆盖基类的 FreeObject 方法。
     * 
     * @param pObj 指向被释放的 IComponent 对象，一般是 Packet 对象。
     */
    virtual void FreeObject(IComponent* pObj) override;

    /**
     * @brief 显示对象池当前的状态信息，覆盖基类中的 Show 方法。
     *        可以用于调试或监控池中对象的使用情况。
     */
    virtual void Show() override;

private:
    // 互斥锁，保护 Packet 对象的分配和释放操作，确保线程安全。
    std::mutex _packet_lock;
};
