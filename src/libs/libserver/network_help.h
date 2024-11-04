#pragma once
#include "common.h"
#include "network_type.h"

class Packet;

/**
 * @brief INetwork 接口类，定义了网络类的基础行为接口。
 * 
 * 任何继承 INetwork 的类必须实现 `SendPacket` 方法，用于发送数据包。
 */
class INetwork
{
public:
    virtual ~INetwork() = default;  // 虚析构函数，保证继承的子类可以正确析构

    /**
     * @brief 纯虚函数，发送数据包的接口。
     * 
     * @param pPacket 要发送的 Packet 包指针。
     */
    virtual void SendPacket(Packet*& pPacket) = 0;
};

/**
 * @brief NetworkHelp 工具类，提供了一些网络相关的静态辅助函数。
 * 
 * 包含判断网络类型、消息类型以及错误处理等常用工具函数。
 */
class NetworkHelp
{
public:
    /**
     * @brief 判断网络类型是否为 TCP。
     * 
     * @param iType 网络类型枚举值。
     * @return bool 如果网络类型为 TCP 连接器或 TCP 监听器，返回 true；否则返回 false。
     */
    inline static bool IsTcp(NetworkType iType)
    {
        return iType == NetworkType::TcpConnector || iType == NetworkType::TcpListen;
    }

    /**
     * @brief 判断消息 ID 是否为 HTTP 消息。
     * 
     * @param msgId 消息的枚举 ID。
     * @return bool 如果消息 ID 对应 HTTP 请求或响应，返回 true；否则返回 false。
     */
    inline static bool IsHttpMsg(int msgId)
    {
        // 判断是否为外部 HTTP 请求消息
        if (msgId == Proto::MsgId::MI_HttpOuterRequest)
            return true;

        // 判断是否为外部 HTTP 响应消息
        if (msgId == Proto::MsgId::MI_HttpOuterResponse)
            return true;

        // 判断是否为内部 HTTP 响应消息
        if (msgId == Proto::MsgId::MI_HttpInnerResponse)
            return true;

        return false;  // 如果不符合以上条件，返回 false
    }

    /**
     * @brief 判断是否是套接字错误。
     * 
     * @param socketError 套接字错误码。
     * @return bool 如果错误码代表非致命错误，返回 false；否则返回 true。
     * 
     * 非 Windows 平台和 Windows 平台的错误码判断略有不同。
     */
    inline static bool IsError(int socketError)
    {
#if ENGINE_PLATFORM != PLATFORM_WIN32
        // EINPROGRESS : 操作正在进行中，表示非阻塞操作的合法状态
        if (socketError == EINTR || socketError == EWOULDBLOCK || socketError == EAGAIN || socketError == EINPROGRESS)
            return false;
#else
        // Windows 平台的非致命错误码
        if (socketError == WSAEINTR || socketError == WSAEWOULDBLOCK || socketError == WSAEINPROGRESS)
            return false;
#endif
        return true;  // 其他错误码为致命错误
    }
};
