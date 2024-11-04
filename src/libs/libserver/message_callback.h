#pragma once
#include "packet.h"

// 消息回调接口
class IMessageCallBack : public Component<IMessageCallBack>
{
public:
    virtual ~IMessageCallBack() = default; // 虚析构函数
    virtual bool ProcessPacket(Packet* packet) = 0; // 处理数据包的纯虚函数
};

// 定义消息回调函数类型
using MsgCallbackFun = std::function<void(Packet*)>;

// 消息回调实现
class MessageCallBack : public IMessageCallBack, public IAwakeFromPoolSystem<MsgCallbackFun>
{
public:
    void Awake(MsgCallbackFun fun) override; // 初始化回调函数
    void BackToPool() override; // 返回池中
    virtual bool ProcessPacket(Packet* pPacket) override; // 处理数据包

private:
    MsgCallbackFun _handleFunction; // 存储回调函数
};

// 带过滤器的消息回调实现
template<class T>
class MessageCallBackFilter : public IMessageCallBack, public IAwakeFromPoolSystem<>
{
public:
    void Awake() override {} // 初始化，空实现
    void BackToPool() override
    {
        HandleFunction = nullptr; // 清空回调函数
        GetFilterObj = nullptr; // 清空过滤器对象
    }

    std::function<void(T*, Packet*)> HandleFunction{ nullptr }; // 处理函数
    std::function<T* (NetIdentify*)> GetFilterObj{ nullptr }; // 过滤器对象获取函数

    virtual bool ProcessPacket(Packet* pPacket) override
    {
        auto pObj = GetFilterObj(pPacket); // 通过数据包获取过滤器对象
        if (pObj == nullptr)
            return false; // 如果未找到对象，返回false

#ifdef LOG_TRACE_COMPONENT_OPEN
        // 记录处理信息
        const google::protobuf::EnumDescriptor* descriptor = Proto::MsgId_descriptor();
        const auto name = descriptor->FindValueByNumber(pPacket->GetMsgId())->name();

        const auto traceMsg = std::string("process. ")
            .append(" sn:").append(std::to_string(pPacket->GetSN()))
            .append(" msgId:").append(name);
        ComponentHelp::GetTraceComponent()->Trace(TraceType::Packet, pPacket->GetSocketKey()->Socket, traceMsg);
#endif

        HandleFunction(pObj, pPacket); // 调用处理函数
        return true; // 处理成功
    }
};
