#include "message_system.h"
#include <utility>
#include "system_manager.h"
#include "packet.h"
#include "entity_system.h"
#include "component.h"
#include "object_pool_packet.h"
#include "component_help.h"

// 消息系统的构造函数，接受系统管理指针
MessageSystem::MessageSystem(SystemManager* pMgr)
{
    _systemMgr = pMgr; // 保存系统管理指针
}

// 释放资源
void MessageSystem::Dispose()
{
    // 当前没有实现清理逻辑
}

// 将数据包添加到缓存列表中
void MessageSystem::AddPacketToList(Packet* pPacket)
{
    std::lock_guard<std::mutex> guard(_packet_lock); // 保护数据包缓存的互斥锁
    _cachePackets.GetWriterCache()->emplace_back(pPacket); // 将数据包添加到写缓存中

    // 引用计数加1
    pPacket->AddRef();
}

// 注册消息回调函数
void MessageSystem::RegisterFunction(IEntity* obj, int msgId, MsgCallbackFun cbfun)
{
    // 查找消息ID对应的回调列表
    auto iter = _callbacks.find(msgId);
    if (iter == _callbacks.end())
    {
        // 如果不存在，则插入新的回调列表
        _callbacks.insert(std::make_pair(msgId, new std::map<uint64, IMessageCallBack*>()));
    }

    // 创建消息回调对象
    const auto pCallback = _systemMgr->GetEntitySystem()->AddComponent<MessageCallBack>(std::move(cbfun));
    pCallback->SetParent(obj); // 设置父对象

    // 将回调对象插入到消息ID对应的回调列表中
    _callbacks[msgId]->insert(std::make_pair(obj->GetSN(), pCallback));
}

// 注册默认消息回调函数
void MessageSystem::RegisterDefaultFunction(IEntity* obj, MsgCallbackFun cbfun)
{
    // 创建默认消息回调对象
    const auto pCallback = _systemMgr->GetEntitySystem()->AddComponent<MessageCallBack>(std::move(cbfun));
    pCallback->SetParent(obj); // 设置父对象

    // 将默认回调对象插入到默认回调列表中
    _defaultCallbacks.insert(std::make_pair(obj->GetSN(), pCallback));
}

// 移除注册的回调函数
void MessageSystem::RemoveFunction(IComponent* obj)
{
    // 遍历所有注册的回调
    for (auto iter1 = _callbacks.begin(); iter1 != _callbacks.end(); ++iter1)
    {
        auto pSub = iter1->second;
        const auto iter2 = pSub->find(obj->GetSN()); // 查找要移除的对象
        if (iter2 == pSub->end())
            continue; // 如果未找到则继续下一个

        pSub->erase(iter2); // 移除回调
    }

    // 移除默认回调
    _defaultCallbacks.erase(obj->GetSN());
}

// 更新消息系统，处理缓存中的数据包
void MessageSystem::Update(EntitySystem* pEntities)
{
    _packet_lock.lock(); // 加锁以保护缓存的安全
    if (_cachePackets.CanSwap())
    {
        _cachePackets.Swap(); // 如果可以交换，则进行交换
    }
    _packet_lock.unlock(); // 解锁

    // 如果读取缓存为空，直接返回
    if (_cachePackets.GetReaderCache()->empty())
        return;

    auto packetLists = _cachePackets.GetReaderCache(); // 获取读取缓存
    for (auto iter = packetLists->begin(); iter != packetLists->end(); ++iter)
    {
        auto pPacket = (*iter); // 当前处理的数据包
        uint64 entitySn = 0; // 实体序列号
        auto pTags = pPacket->GetTagKey(); // 获取数据包的标签
        const auto pTagValue = pTags->GetTagValue(TagType::Entity); // 获取实体标签值
        if (pTagValue != nullptr)
        {
            entitySn = pTagValue->KeyInt64; // 获取实体序列号
        }

        const auto msgIterator = _callbacks.find(pPacket->GetMsgId()); // 查找消息ID对应的回调
        bool isDo = false; // 是否处理过标志

        if (msgIterator != _callbacks.end())
        {
            auto pSub = msgIterator->second; // 获取回调子集
            if (entitySn > 0)
            {
                const auto iterSub = pSub->find(entitySn); // 查找实体序列号对应的回调
                if (iterSub != pSub->end())
                {
                    // 如果处理成功，设置标志
                    if (iterSub->second->ProcessPacket(pPacket))
                        isDo = true;
                }
            }
            else
            {
                // 如果没有实体序列号，则遍历所有回调
                for (auto iterSub = pSub->begin(); iterSub != pSub->end(); ++iterSub)
                {
                    if (iterSub->second->ProcessPacket(pPacket))
                        isDo = true; // 标记为已处理
                }
            }
        }

        // 如果没有处理，检查是否有默认回调
        if (!isDo)
        {
            if (entitySn > 0)
            {
                const auto pTagToWorld = pTags->GetTagValue(TagType::ToWorld); // 检查是否有世界标签
                if (pTagToWorld == nullptr)
                {
                    auto pMsgCallback = _defaultCallbacks[entitySn]; // 获取默认回调
                    if (pMsgCallback != nullptr)
                    {
                        pMsgCallback->ProcessPacket(pPacket); // 处理数据包
                    }
                }
            }
        }

        // 处理完成后，引用计数减1
        pPacket->RemoveRef();
    }

    _cachePackets.GetReaderCache()->clear(); // 清空读取缓存
}
