#include "create_component.h"
#include "common.h"
#include "component_factory.h"
#include "packet.h"
#include "entity.h"
#include "system_manager.h"
#include "thread.h"

// 模板函数：根据参数元组动态调用 AddComponentByName
template <typename... TArgs, size_t... Index>
IComponent* ComponentFactoryEx(EntitySystem* pEntitySystem, std::string className, uint64 sn, const std::tuple<TArgs...>& args, std::index_sequence<Index...>)
{
    // 调用 AddComponentByName，解包参数
    return pEntitySystem->AddComponentByName(className, sn, std::get<Index>(args)...);
}

// 递归结构体：动态调用的实现
template<size_t ICount>
struct DynamicCall
{
    template <typename... TArgs>
    static IComponent* Invoke(EntitySystem* pEntitySystem, const std::string classname, uint64 sn, std::tuple<TArgs...> t1, google::protobuf::RepeatedPtrField<::Proto::CreateComponentParam>& params)
    {
        // 如果没有参数，直接调用 ComponentFactoryEx
        if (params.empty())
        {
            return ComponentFactoryEx(pEntitySystem, classname, sn, t1, std::make_index_sequence<sizeof...(TArgs)>());
        }

        // 从参数列表中获取第一个参数
        Proto::CreateComponentParam param = (*(params.begin()));
        params.erase(params.begin()); // 移除第一个参数

        // 根据参数类型递归调用
        if (param.type() == Proto::CreateComponentParam::Int)
        {
            auto t2 = std::tuple_cat(t1, std::make_tuple(param.int_param()));
            return DynamicCall<ICount - 1>::Invoke(pEntitySystem, classname, sn, t2, params);
        }

        if (param.type() == Proto::CreateComponentParam::String)
        {
            auto t2 = std::tuple_cat(t1, std::make_tuple(param.string_param()));
            return DynamicCall<ICount - 1>::Invoke(pEntitySystem, classname, sn, t2, params);
        }

        if (param.type() == Proto::CreateComponentParam::UInt64)
        {
            auto t2 = std::tuple_cat(t1, std::make_tuple(param.uint64_param()));
            return DynamicCall<ICount - 1>::Invoke(pEntitySystem, classname, sn, t2, params);
        }

        return nullptr; // 无法处理的参数类型
    }
};

// 基础情况：当没有参数时的处理
template<>
struct DynamicCall<0>
{
    template <typename... TArgs>
    static IComponent* Invoke(EntitySystem* pEntitySystem, const std::string classname, uint64 sn, std::tuple<TArgs...> t1, google::protobuf::RepeatedPtrField<::Proto::CreateComponentParam>& params)
    {
        return nullptr; // 没有参数，返回 nullptr
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Awake 函数：注册消息处理函数
void CreateComponentC::Awake()
{
    auto pMsgSystem = GetSystemManager()->GetMessageSystem();

    // 注册创建组件的处理函数
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_CreateComponent, BindFunP1(this, &CreateComponentC::HandleCreateComponent));
    // 注册移除组件的处理函数
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_RemoveComponent, BindFunP1(this, &CreateComponentC::HandleRemoveComponent));
    // 注册创建系统的处理函数
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_CreateSystem, BindFunP1(this, &CreateComponentC::HandleCreateSystem));
}

// BackToPool 函数：处理回收逻辑（未实现）
void CreateComponentC::BackToPool()
{
    // 这里可以添加组件回收的逻辑
}

// 最大动态调用参数数量
#define MaxDynamicCall 4

// 处理创建组件的消息
void CreateComponentC::HandleCreateComponent(Packet* pPacket) const
{
    Proto::CreateComponent proto = pPacket->ParseToProto<Proto::CreateComponent>();
    const std::string className = proto.class_name(); // 获取类名
    uint64 sn = proto.sn(); // 获取序列号

    // 检查参数数量是否超过最大限制
    if (proto.params_size() > MaxDynamicCall)
    {
        LOG_ERROR(" !!!! CreateComponent failed. className:" << className.c_str() << " params size > " << MaxDynamicCall);
        return;
    }

    auto params = proto.params(); // 获取参数
    const auto pObj = DynamicCall<MaxDynamicCall>::Invoke(GetSystemManager()->GetEntitySystem(), className, sn, std::make_tuple(), params);
    if (pObj == nullptr)
    {
        LOG_ERROR(" !!!! CreateComponent failed. className:" << className.c_str());
    }

    // std::cout << "CreateComponent. name:" << className << std::endl; // 可选调试输出
}

// 处理移除组件的消息
void CreateComponentC::HandleRemoveComponent(Packet* pPacket)
{
    Proto::RemoveComponent proto = pPacket->ParseToProto<Proto::RemoveComponent>();
    // GetEntitySystem()->RemoveFromSystem(proto.sn()); // 逻辑未实现
}

// 处理创建系统的消息
void CreateComponentC::HandleCreateSystem(Packet* pPacket)
{
    Proto::CreateSystem proto = pPacket->ParseToProto<Proto::CreateSystem>();
    const std::string systemName = proto.system_name(); // 获取系统名

    const auto pThread = static_cast<Thread*>(GetSystemManager());
    if (int(pThread->GetThreadType()) != proto.thread_type()) // 检查线程类型
        return;

    GetSystemManager()->AddSystem(systemName); // 添加系统
}
