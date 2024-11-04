#pragma once

#include <mutex>
#include <vector>

#include "common.h"
#include "thread.h"
#include "cache_swap.h"
#include "singleton.h"
#include "entity_system.h"
#include "component_factory.h"
#include "regist_to_factory.h"
#include "message_system_help.h"
#include "thread_collector.h"
#include "thread_type.h"

// 线程管理类
class ThreadMgr : public Singleton<ThreadMgr>, public SystemManager
{
public:
    ThreadMgr();
    void InitializeThread();  // 初始化线程
    void CreateThread(ThreadType iType, int num);  // 创建指定类型的线程

    void InitializeGlobalComponent(APP_TYPE ppType, int appId);  // 初始化全局组件
    void Update() override;  // 更新函数
    void UpdateCreatePacket();  // 更新创建数据包
    void UpdateDispatchPacket();  // 更新分发数据包

    bool IsStopAll();  // 判断是否停止所有线程
    void DestroyThread();  // 销毁线程
    bool IsDestroyAll();  // 判断是否销毁所有线程

    void Dispose() override;  // 清理资源

    // 创建系统
    template<class T, typename ...TArgs>
    void CreateSystem(TArgs... args);

    // 创建组件
    template<class T, typename ...TArgs>
    void CreateComponent(TArgs... args);

    // 通过序列号创建组件
    template<class T, typename ...TArgs>
    void CreateComponentWithSn(uint64 sn, TArgs... args);

    // 创建组件，指定是否分配到所有线程
    template<class T, typename ...TArgs>
    void CreateComponent(bool isToAllThead, TArgs... args);

    // 创建组件，指定线程类型和是否分配到所有线程
    template<class T, typename ...TArgs>
    void CreateComponent(ThreadType iType, bool isToAllThead, TArgs... args);

    // 消息分发
    void DispatchPacket(Packet* pPacket);

private:
    // 通过线程类型、序列号、是否分配到所有线程创建组件
    template<class T, typename ...TArgs>
    void CreateComponentWithSn(ThreadType iType, uint64 sn, bool isToAllThead, TArgs... args);

    // 参数分析
    template <typename...Args>
    void AnalyseParam(Proto::CreateComponent& proto, int value, Args...args);
    template <typename...Args>
    void AnalyseParam(Proto::CreateComponent& proto, std::string value, Args...args);
    template <typename...Args>
    void AnalyseParam(Proto::CreateComponent& proto, uint64 value, Args...args);

    // 参数分析结束
    void AnalyseParam(Proto::CreateComponent& proto) {}

private:
    std::map<ThreadType, ThreadCollector*> _threads;  // 线程集合

    // 创建信息锁
    std::mutex _create_lock;
    CacheSwap<Packet> _createPackets;  // 创建数据包缓存

    // 数据包锁
    std::mutex _packet_lock;
    CacheSwap<Packet> _packets;  // 数据包缓存
};

// 创建系统模板函数
template <class T, typename ... TArgs>
void ThreadMgr::CreateSystem(TArgs... args)
{
    const std::string className = typeid(T).name();  // 获取类名
    if (!ComponentFactory<TArgs...>::GetInstance()->IsRegisted(className))
    {
        RegistObject<T, TArgs...>();  // 注册对象
    }

    Proto::CreateSystem proto;
    proto.set_system_name(className.c_str());  // 设置系统名称
    proto.set_thread_type((int)LogicThread);  // 设置线程类型

    auto pCreatePacket = MessageSystemHelp::CreatePacket(Proto::MsgId::MI_CreateSystem, nullptr);
    pCreatePacket->SerializeToBuffer(proto);  // 序列化数据包
    DispatchPacket(pCreatePacket);  // 分发数据包
}

// 创建组件模板函数
template<class T, typename ...TArgs>
void ThreadMgr::CreateComponent(TArgs ...args)
{
    CreateComponent<T>(LogicThread, false, std::forward<TArgs>(args)...);  // 默认使用逻辑线程
}

// 通过序列号创建组件模板函数
template <class T, typename ... TArgs>
void ThreadMgr::CreateComponentWithSn(uint64 sn, TArgs... args)
{
    CreateComponentWithSn<T>(LogicThread, sn, false, std::forward<TArgs>(args)...);
}

// 创建组件模板函数，指定是否分配到所有线程
template<class T, typename ...TArgs>
void ThreadMgr::CreateComponent(bool isToAllThead, TArgs ...args)
{
    CreateComponent<T>(LogicThread, isToAllThead, std::forward<TArgs>(args)...);
}

// 创建组件模板函数，指定线程类型和是否分配到所有线程
template<class T, typename ...TArgs>
void ThreadMgr::CreateComponent(ThreadType iType, bool isToAllThead, TArgs ...args)
{
    CreateComponentWithSn<T>(iType, static_cast<uint64>(0), isToAllThead, std::forward<TArgs>(args)...);
}

// 通过线程类型、序列号、是否分配到所有线程创建组件模板函数
template <class T, typename ... TArgs>
void ThreadMgr::CreateComponentWithSn(ThreadType iType, uint64 sn, bool isToAllThead, TArgs... args)
{
    std::lock_guard<std::mutex> guard(_create_lock);  // 加锁以保证线程安全

    const std::string className = typeid(T).name();
    if (!ComponentFactory<TArgs...>::GetInstance()->IsRegisted(className))
    {
        RegistToFactory<T, TArgs...>();  // 注册到工厂
    }

    Proto::CreateComponent proto;
    proto.set_thread_type((int)iType);  // 设置线程类型
    proto.set_sn(sn);  // 设置序列号
    proto.set_class_name(className.c_str());  // 设置类名
    proto.set_is_to_all_thread(isToAllThead);  // 设置是否分配到所有线程
    AnalyseParam(proto, std::forward<TArgs>(args)...);  // 分析参数

    auto pCreatePacket = MessageSystemHelp::CreatePacket(Proto::MsgId::MI_CreateComponent, nullptr);
    pCreatePacket->SerializeToBuffer(proto);  // 序列化数据包
    _createPackets.GetWriterCache()->emplace_back(pCreatePacket);  // 将数据包放入缓存
}

// 分析整型参数
template<typename ... Args>
void ThreadMgr::AnalyseParam(Proto::CreateComponent& proto, int value, Args... args)
{
    auto pProtoParam = proto.mutable_params()->Add();
    pProtoParam->set_type(Proto::CreateComponentParam::Int);
    pProtoParam->set_int_param(value);  // 设置整型参数
    AnalyseParam(proto, std::forward<Args>(args)...);  // 递归分析下一个参数
}

// 分析字符串参数
template<typename ... Args>
void ThreadMgr::AnalyseParam(Proto::CreateComponent& proto, std::string value, Args... args)
{
    auto pProtoParam = proto.mutable_params()->Add();
    pProtoParam->set_type(Proto::CreateComponentParam::String);
    pProtoParam->set_string_param(value.c_str());  // 设置字符串参数
    AnalyseParam(proto, std::forward<Args>(args)...);  // 递归分析下一个参数
}

// 分析无符号整型参数
template <typename ... Args>
void ThreadMgr::AnalyseParam(Proto::CreateComponent& proto, uint64 value, Args... args)
{
    auto pProtoParam = proto.mutable_params()->Add();
    pProtoParam->set_type(Proto::CreateComponentParam::UInt64);
    pProtoParam->set_uint64_param(value);  // 设置无符号整型参数
    AnalyseParam(proto, std::forward<Args>(args)...);  // 递归分析下一个参数
}
