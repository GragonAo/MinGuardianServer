#include "message_callback.h"

// 唤醒回调，设置处理函数
void MessageCallBack::Awake(MsgCallbackFun fun)
{
    _handleFunction = fun;  // 保存传入的处理函数
}

// 将回调对象返回池中，清空处理函数
void MessageCallBack::BackToPool()
{
    _handleFunction = nullptr;  // 清空处理函数，准备重用
}

// 处理接收到的数据包
bool MessageCallBack::ProcessPacket(Packet* pPacket)
{
#ifdef LOG_TRACE_COMPONENT_OPEN
    // 获取消息 ID 的描述符
    const google::protobuf::EnumDescriptor* descriptor = Proto::MsgId_descriptor();
    // 根据消息 ID 获取对应的名称
    const auto name = descriptor->FindValueByNumber(pPacket->GetMsgId())->name();

    // 生成追踪日志信息
    const auto traceMsg = std::string("process. ")
        .append(" sn:").append(std::to_string(pPacket->GetSN()))  // 添加序列号
        .append(" msgId:").append(name);  // 添加消息 ID 名称
    
    // 记录追踪信息
    ComponentHelp::GetTraceComponent()->Trace(TraceType::Packet, pPacket->GetSocketKey()->Socket, traceMsg);
#endif

    // 调用处理函数处理数据包
    _handleFunction(pPacket);
    return true;  // 返回处理成功标志
}
