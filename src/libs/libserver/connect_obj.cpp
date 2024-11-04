#include "connect_obj.h"

#include "network.h"
#include "network_buffer.h"

#include "thread_mgr.h"

#include "system_manager.h"
#include "message_system.h"
#include "message_system_help.h"
#include "component_help.h"

#include "object_pool_packet.h"
#include "network_locator.h"
#include "socket_locator.h"

ConnectObj::ConnectObj()
{
    _state = ConnectStateType::None; // 初始化连接状态为无
    _recvBuffer = new RecvNetworkBuffer(DEFAULT_RECV_BUFFER_SIZE, this); // 初始化接收缓冲区
    _sendBuffer = new SendNetworkBuffer(DEFAULT_SEND_BUFFER_SIZE, this); // 初始化发送缓冲区
}

ConnectObj::~ConnectObj()
{
    delete _recvBuffer; // 删除接收缓冲区
    delete _sendBuffer; // 删除发送缓冲区
}

void ConnectObj::Awake(SOCKET socket, NetworkType networkType, TagType tagType, TagValue tagValue, ConnectStateType state)
{
    _socketKey = SocketKey(socket, networkType); // 设置套接字键

    _tagKey.Clear(); // 清空标签
    if (tagType != TagType::None)
        _tagKey.AddTag(tagType, tagValue); // 添加标签

    _state = state; // 设置连接状态
}

void ConnectObj::BackToPool()
{
#ifdef LOG_TRACE_COMPONENT_OPEN
    const auto traceMsg = std::string("close.  network type:").append(GetNetworkTypeName(_socketKey.NetType));
    ComponentHelp::GetTraceComponent()->Trace(TraceType::Connector, _socketKey.Socket, traceMsg);
#endif

    // 通知系统，准备关闭连接
    MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_NetworkDisconnect, this);    

    if (_socketKey.Socket != INVALID_SOCKET)
        _sock_close(_socketKey.Socket); // 关闭套接字

    _state = ConnectStateType::None; // 重置状态
    _socketKey.Clear(); // 清空套接字键
    _tagKey.Clear(); // 清空标签

    _recvBuffer->BackToPool(); // 将接收缓冲区返回池中
    _sendBuffer->BackToPool(); // 将发送缓冲区返回池中
}

bool ConnectObj::HasRecvData() const
{
    return _recvBuffer->HasData(); // 检查是否有接收数据
}

bool ConnectObj::Recv()
{
    if (_state == ConnectStateType::Connecting)
    {
        ChangeStateToConnected(); // 改变状态为已连接
        return true;
    }

    bool isRs = false;
    char* pBuffer = nullptr;
    while (true)
    {
        // 检查缓冲区是否有足够的空间接收数据
        if (_recvBuffer->GetEmptySize() < (sizeof(PacketHeadS2S) + sizeof(TotalSizeType) * 2))
        {
            _recvBuffer->ReAllocBuffer(); // 重新分配缓冲区
        }

        const int emptySize = _recvBuffer->GetBuffer(pBuffer);
        const int dataSize = ::recv(_socketKey.Socket, pBuffer, emptySize, 0);
        if (dataSize > 0)
        {
            _recvBuffer->FillDate(dataSize); // 填充接收到的数据
            isRs = true;
        }
        else if (dataSize == 0)
        {
            break; // 连接已关闭
        }
        else
        {
            const auto socketError = _sock_err();
            isRs = socketError != 0;
            if (!NetworkHelp::IsError(socketError))
            {
                isRs = true; // 处理非错误情况
            }

            if (!isRs)
                LOG_WARN("recv size:" << dataSize << " error:" << socketError); // 日志记录接收错误

            break;
        }
    }

    if (isRs)
    {
        const auto pNetwork = this->GetParent<Network>();
        const auto iNetworkType = pNetwork->GetNetworkType();
        while (true)
        {
            const auto pPacket = _recvBuffer->GetPacket(); // 获取数据包
            if (pPacket == nullptr)
                break;

            const auto pTagAccount = this->_tagKey.GetTagValue(TagType::Account);
            if (pTagAccount != nullptr)
            {
                auto pSocketLocator = ComponentHelp::GetGlobalEntitySystem()->GetComponent<SocketLocator>();
                if (pSocketLocator != nullptr)
                {
                    const auto targetEntitySn = pSocketLocator->GetTargetEntitySn(pPacket->GetSocketKey()->Socket);
                    if (targetEntitySn > 0)
                    {
                        pPacket->GetTagKey()->AddTag(TagType::Entity, targetEntitySn); // 添加实体标签
                    }
                }
            }

            const auto msgId = pPacket->GetMsgId(); // 获取消息ID
            const bool isTcp = NetworkHelp::IsTcp(iNetworkType);
            if (!isTcp)
            {
                if (msgId == Proto::MsgId::MI_HttpRequestBad)
                {
                    // 处理404错误
                    MessageSystemHelp::SendHttpResponse404(pPacket);
                    DynamicPacketPool::GetInstance()->FreeObject(pPacket);
                    continue;
                }
            }
            else
            {
                if (msgId == Proto::MsgId::MI_Ping)
                {
                    // 处理Ping消息
                    continue;
                }
            }

            if (!isTcp)
            {
                if ((msgId <= Proto::MsgId::MI_HttpBegin || msgId >= Proto::MsgId::MI_HttpEnd) && msgId != Proto::MsgId::MI_HttpOuterResponse)
                {
                    // 非HTTP协议的错误
                    LOG_WARN("http connect recv. tcp proto");
                    DynamicPacketPool::GetInstance()->FreeObject(pPacket);
                    continue;
                }
            }

            ThreadMgr::GetInstance()->DispatchPacket(pPacket); // 分发数据包
        }
    }

    return isRs; // 返回接收状态
}

bool ConnectObj::HasSendData() const
{
    if (_state == ConnectStateType::Connecting)
        return true; // 连接中时，始终返回有数据可发送

    return _sendBuffer->HasData(); // 检查是否有待发送数据
}

void ConnectObj::SendPacket(Packet* pPacket) const
{
    _sendBuffer->AddPacket(pPacket); // 将数据包添加到发送缓冲区
    DynamicPacketPool::GetInstance()->FreeObject(pPacket); // 释放数据包
}

bool ConnectObj::Send()
{
    if (_state == ConnectStateType::Connecting)
    {
        ChangeStateToConnected(); // 改变状态为已连接
        return true;
    }

    while (true) {
        char* pBuffer = nullptr;
        const int needSendSize = _sendBuffer->GetBuffer(pBuffer); // 获取待发送数据的大小

        // 无需发送数据
        if (needSendSize <= 0)
        {
            return true;
        }

        const int size = ::send(_socketKey.Socket, pBuffer, needSendSize, 0);
        if (size > 0)
        {
            _sendBuffer->RemoveDate(size); // 移除已发送的数据

            // 如果发送的大小小于待发送大小，返回
            if (size < needSendSize)
            {
                return true;
            }
        }

        if (size <= 0)
        {
            const auto socketError = _sock_err();
            std::cout << "needSendSize:" << needSendSize << " error:" << socketError << std::endl; // 记录发送错误
            return false;
        }
    }
}

void ConnectObj::Close()
{
    // 创建断开连接的请求数据包
    const auto pPacketDis = MessageSystemHelp::CreatePacket(Proto::MsgId::MI_NetworkRequestDisconnect, this);
    GetSystemManager()->GetMessageSystem()->AddPacketToList(pPacketDis); // 将数据包添加到系统消息列表
    pPacketDis->OpenRef(); // 打开引用计数
}

ConnectStateType ConnectObj::GetState() const
{
    return _state; // 返回连接状态
}

void ConnectObj::ChangeStateToConnected()
{
    _state = ConnectStateType::Connected; // 改变状态为已连接
    const auto pTagValue = _tagKey.GetTagValue(TagType::App);
    if (pTagValue != nullptr)
    {
        auto pLocator = ThreadMgr::GetInstance()->GetEntitySystem()->GetComponent<NetworkLocator>();
        pLocator->AddNetworkIdentify(&_socketKey, pTagValue->KeyInt64); // 添加网络标识
    }
    else
    {
        // 通知系统，连接成功
        MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_NetworkConnected, this);
    }
}
