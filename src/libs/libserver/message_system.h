#pragma once

#include <map>
#include <list>
#include <functional>

#include "common.h"
#include "system.h"
#include "cache_swap.h"
#include "socket_object.h"
#include "component_help.h"
#include "packet.h"
#include "message_callback.h"

// 前向声明类
class IComponent;
class SystemManager;
class Packet;
class EntitySystem;

// 消息系统类，用于处理消息的注册与回调
class MessageSystem : virtual public ISystem<MessageSystem>
{
public:
    // 构造函数，接受系统管理指针
    MessageSystem(SystemManager* pMgr);
    
    // 释放资源
    void Dispose() override;

    // 更新消息系统
    void Update(EntitySystem* pEntities) override;

    // 添加数据包到列表
    void AddPacketToList(Packet* pPacket);

    // 注册消息回调函数
    void RegisterFunction(IEntity* obj, int msgId, MsgCallbackFun cbfun);
    
    // 注册默认消息回调函数
    void RegisterDefaultFunction(IEntity* obj, MsgCallbackFun cbfun);
    
    // 移除注册的回调函数
    void RemoveFunction(IComponent* obj);

    // 注册带过滤器的消息回调
    template<typename T>
    void RegisterFunctionFilter(IEntity* obj, int msgId, std::function<T* (NetIdentify*)> getObj, std::function<void(T*, Packet*)> fun);

private:
    // 处理消息包的互斥锁
    std::mutex _packet_lock; 
    
    // 缓存交换器，用于存储数据包
    CacheSwap<Packet> _cachePackets; 

    // 系统管理指针
    SystemManager* _systemMgr{ nullptr };

    // 消息回调存储
    // 结构为 <msgId, <objsn, callback>>
    std::map<int, std::map<uint64, IMessageCallBack*>*> _callbacks;

    // 默认回调存储
    // 结构为 <objsn, callback>
    std::map<uint64, IMessageCallBack*> _defaultCallbacks;
};

// 注册带过滤器的消息回调
template <typename T>
void MessageSystem::RegisterFunctionFilter(IEntity* obj, int msgId, std::function<T* (NetIdentify*)> getObj, std::function<void(T*, Packet*)> fun)
{
    // 查找消息ID对应的回调列表
    auto iter = _callbacks.find(msgId);
    if (iter == _callbacks.end())
    {
        // 如果不存在，则插入新的回调列表
        _callbacks.insert(std::make_pair(msgId, new std::map<uint64, IMessageCallBack*>()));
    }

    // 创建消息回调过滤器组件
    auto pCallback = _systemMgr->GetEntitySystem()->AddComponent<MessageCallBackFilter<T>>();
    pCallback->GetFilterObj = std::move(getObj); // 设置获取对象的函数
    pCallback->HandleFunction = std::move(fun);  // 设置处理函数

    pCallback->SetParent(obj); // 设置父对象

    // 将回调函数插入到消息ID对应的回调列表中
    _callbacks[msgId]->insert(std::make_pair(obj->GetSN(), pCallback));
}
