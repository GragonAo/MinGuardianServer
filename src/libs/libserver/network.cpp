#include "network.h"
#include "connect_obj.h"
#include "libserver/network_type.h"
#include "libserver/socket_object.h"
#include "packet.h"
#include "common.h"
#include "object_pool.h"
#include <iostream>
#include "component_help.h"
#include "object_pool_packet.h"

// 复用池资源时的回调函数，用于将网络对象重置以备后续复用
void Network::BackToPool()
{
    // 将发送消息列表中的所有消息重置到对象池中
    _sendMsgList.BackToPool();

    // 遍历所有 socket，断开连接并重置连接对象
    for(auto socket:_sockets)
    {
        auto pObj = _connects[socket];
        pObj->ComponentBackToPool(); // 将连接对象返回到对象池
        _connects[socket] = nullptr; // 清除连接对象指针
    }
    _sockets.clear(); // 清空 sockets 集合

#ifdef EPOLL
    // 如果使用了 EPOLL，则关闭 epoll 文件描述符
    ::close(_epfd);
    _epfd = -1;
#endif
}

// 设置 socket 选项，如超时和 keep-alive 配置
void Network::SetSocketOpt(SOCKET socket)
{
    // 1. 设置发送和接收的超时，单位为毫秒
    int netTimeout = 10000; // 超时设置为 3 秒
    setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (SetsockOptType)&netTimeout, sizeof(netTimeout));
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (SetsockOptType)&netTimeout, sizeof(netTimeout));

#if ENGINE_PLATFORM != PLATFORM_WIN32
    // 如果不是 HTTP 网络类型，设置 keep-alive 选项以保证长连接的存活
    if (_networkType != NetworkType::HttpConnector && _networkType != NetworkType::HttpListen)
    {
        int keepAlive = 1; // 开启 keep-alive
        socklen_t optlen = sizeof(keepAlive);

        // 设置 keep-alive 的参数，包括空闲时间、检测间隔和最大重试次数
        int keepIdle = 60 * 2;     // 2 分钟无数据传输后启动 keep-alive
        int keepInterval = 10;     // 每隔 10 秒发送一次探测数据包
        int keepCount = 5;         // 如果 5 次探测失败则断开连接

        setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, (SetsockOptType)&keepAlive, optlen);
        if (getsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, &optlen) < 0)
        {
            LOG_WARN("获取 SO_KEEPALIVE 失败. 错误码:" << _sock_err() << " socket:" << socket << " 网络类型:" << GetNetworkTypeName(_networkType));
        }

        setsockopt(socket, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, optlen);
        if (getsockopt(socket, SOL_TCP, TCP_KEEPIDLE, &keepIdle, &optlen) < 0)
        {
            LOG_WARN("获取 TCP_KEEPIDLE 失败. 错误码:" << _sock_err() << " socket:" << socket << " 网络类型:" << GetNetworkTypeName(_networkType));
        }

        setsockopt(socket, SOL_TCP, TCP_KEEPINTVL, (void*)&keepInterval, optlen);
        setsockopt(socket, SOL_TCP, TCP_KEEPCNT, (void*)&keepCount, optlen);
    }else{
        // 启用 TCP_NODELAY 以减少延迟
        int flag = 1;
        setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
    }
#endif

    // 设置 socket 为非阻塞模式
    _sock_nonblock(socket);
}

// 创建 socket，返回对应的 socket 描述符
SOCKET Network::CreateSocket()
{
    _sock_init(); // 初始化 socket 库（在 Windows 上有作用）

    SOCKET socket;
    // 根据网络类型创建不同的 socket，HTTP 使用 IPPROTO_IP，其它使用 IPPROTO_TCP
    if (_networkType == NetworkType::HttpListen || _networkType == NetworkType::HttpConnector)
        socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    else
        socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (socket == INVALID_SOCKET)
    {
        std::cout << "::socket 创建失败. 错误码:" << _sock_err() << std::endl;
        return socket;
    }

    // 设置 socket 选项
    SetSocketOpt(socket);
    return socket;
}

// 检查给定的 socket 是否有效（如连接状态等），如果无效则关闭 socket
bool Network::CheckSocket(SOCKET socket)
{
    // 通过 getsockopt 获取 socket 的错误状态
    int err = EBADF; // EBADF 表示无效文件描述符错误
    socklen_t len = sizeof(err);
    if (::getsockopt(socket, SOL_SOCKET, SO_ERROR, (char*)(&err), &len) == 0)
    {
        // 如果没有错误则 err 为 0
        if (!NetworkHelp::IsError(err))
        {
            err = 0;
        }
    }

    if (err != 0)
    {
        _sock_close(socket); // 如果有错误，关闭该 socket
        return false;
    }

    return true; // socket 状态良好，返回 true
}

// 为 socket 创建对应的连接对象 `ConnectObj`
bool Network::CreateConnectObj(SOCKET socket, TagType tagType,TagValue tagValue, ConnectStateType iState)
{
    if (!CheckSocket(socket))
        return false; // 如果 socket 无效，返回失败

    // 检查 socket 是否已经存在连接对象，如果有则需要先移除旧的连接对象
    if (_connects[socket] != nullptr)
    {
        LOG_ERROR("Network::CreateConnectObj. socket 已经存在. socket:" << socket << " sn:" << _sn);
        RemoveConnectObj(socket); // 移除现有连接对象
        return false;
    }

    // 从对象池中分配一个新的 `ConnectObj`
    auto pCollector = _pSystemManager->GetPoolCollector();
    auto pPool = dynamic_cast<DynamicObjectPool<ConnectObj>*>(pCollector->GetPool<ConnectObj>());
    ConnectObj* pConnectObj = pPool->MallocObject(_pSystemManager, nullptr, 0, socket, _networkType, tagType, tagValue, iState);
    pConnectObj->SetParent(this); // 设置父对象为当前的网络管理器

    // 记录该 socket 的连接对象，并将 socket 加入 _sockets 集合
    _connects[socket] = pConnectObj;
    _sockets.emplace(socket);

#ifdef LOG_TRACE_COMPONENT_OPEN
    // 如果开启了日志追踪，则记录创建连接对象的操作
    const auto traceMsg = std::string("create. network type:").append(GetNetworkTypeName(_networkType));
    ComponentHelp::GetTraceComponent()->Trace(TraceType::Connector, socket, traceMsg);
#endif

#ifdef EPOLL
    // 如果使用 EPOLL，则将 socket 注册到 epoll 事件中，监听读写等事件
    AddEvent(_epfd, socket, EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP);
#endif

    return true;
}

// 处理断开连接的逻辑，移除 socket 对应的连接对象
void Network::HandleDisconnect(Packet* pPacket)
{
    const auto socketKey = pPacket->GetSocketKey();
    if (socketKey->NetType != _networkType)
        return; // 如果网络类型不同，直接返回

    const auto socket = socketKey->Socket;
    if (_connects[socket] == nullptr)
    {
        std::cout << "断开连接失败. 未找到 socket 对应的对象." << pPacket << std::endl;
        return;
    }

    RemoveConnectObj(socket); // 移除连接对象
}

#ifdef EPOLL

// 添加 socket 到 epoll 事件中，监听指定的事件类型
void Network::AddEvent(int epollfd, int fd, int flag)
{
    struct epoll_event ev;
    ev.events = flag;
    ev.data.ptr = nullptr;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
}

// 修改 epoll 中 socket 的事件类型
void Network::ModifyEvent(int epollfd, int fd, int flag)
{
    struct epoll_event ev;
    ev.events = flag;
    ev.data.ptr = nullptr;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
}

// 从 epoll 中删除 socket 事件
void Network::DeleteEvent(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, nullptr);
}

// 初始化 epoll，创建 epoll 文件描述符

void Network::InitEpoll()
{
    _epfd = epoll_create(MAX_CLIENT);
}

// 网络事件处理类的实现

// 使用 epoll 机制处理网络事件
void Network::Epoll()
{
    // 等待事件的发生，返回触发事件的 socket 数量
    const int nfds = epoll_wait(_epfd, _events, MAX_EVENT, 0);
    for (int index = 0; index < nfds; index++)
    {
        SOCKET socket = _events[index].data.fd; // 获取触发事件的 socket
        OnEpoll(socket, index); // 处理该 socket 的事件

        // 获取与 socket 相关联的连接对象
        auto pObj = _connects[socket];
        if (pObj == nullptr)
            continue; // 如果连接对象不存在，跳过该循环

        // 检查事件类型，处理断开连接、错误等情况
        if (_events[index].events & EPOLLRDHUP || 
            _events[index].events & EPOLLERR || 
            _events[index].events & EPOLLHUP)
        {
            RemoveConnectObj(socket); // 移除连接对象
            continue;
        }

        // 如果 socket 可读，则接收数据
        if (_events[index].events & EPOLLIN)
        {
            if (!pObj->Recv()) // 调用接收函数，如果失败则移除连接对象
            {
                RemoveConnectObj(socket);
                continue;
            }
        }

        // 如果 socket 可写，则发送数据
        if (_events[index].events & EPOLLOUT)
        {
            if (!pObj->Send()) // 调用发送函数，如果失败则移除连接对象
            {
                RemoveConnectObj(socket);
                continue;
            }

            // 发送成功后，修改事件类型为可读状态
            ModifyEvent(_epfd, socket, EPOLLIN | EPOLLRDHUP);
        }
    }
}

#else

// 使用 select 机制处理网络事件
void Network::Select()
{
    // 遍历所有 socket，设置相应的 fd_set
    for(auto socket:_sockets)
    {
        ConnectObj* pObj = _connects[socket];
        if (socket > _fdMax)
            _fdMax = socket; // 更新最大 socket 值

        FD_SET(socket, &readfds); // 将 socket 添加到可读集合中
        FD_SET(socket, &exceptfds); // 将 socket 添加到异常集合中

        // 如果连接对象有待发送的数据，将 socket 添加到可写集合中
        if (pObj->HasSendData())
            FD_SET(socket, &writefds);
    }

#if LOG_TRACE_COMPONENT_OPEN
    CheckPoint("select begin"); // 记录 select 开始时间
#endif

    struct timeval timeout;
    timeout.tv_sec = 0; // 设置超时为 0 秒
    timeout.tv_usec = 0; // 设置超时为 0 微秒
    const int nfds = ::select(_fdMax + 1, &readfds, &writefds, &exceptfds, &timeout); // 执行 select
    if (nfds <= 0)
        return; // 如果没有可处理的事件，则返回

#if LOG_TRACE_COMPONENT_OPEN
    CheckPoint("select end"); // 记录 select 结束时间
#endif

    auto iter = _sockets.begin(); // 遍历所有 socket
    while (iter != _sockets.end())
    {
        auto socket = *iter; // 获取当前 socket
        auto pObj = _connects[socket]; // 获取连接对象
        if (FD_ISSET(socket, &exceptfds)) // 如果在异常集合中
        {
            std::cout << "socket except!! socket:" << socket << std::endl; // 输出异常信息
            RemoveConnectObjByItem(iter); // 移除该 socket 的连接对象
            continue;
        }

        // 如果在可读集合中，则接收数据
        if (FD_ISSET(socket, &readfds))
        {
            if (!pObj->Recv()) // 调用接收函数，如果失败则移除连接对象
            {
                RemoveConnectObjByItem(iter);
                continue;
            }
        }

        // 如果在可写集合中，则发送数据
        if (FD_ISSET(socket, &writefds))
        {
            if (!pObj->Send()) // 调用发送函数，如果失败则移除连接对象
            {
                RemoveConnectObjByItem(iter);
                continue;
            }
        }

        ++iter; // 移动到下一个 socket
    }
}

#endif

// 更新网络状态，发送待发送的数据包
void Network::OnNetworkUpdate()
{
    _sendMsgMutex.lock(); // 加锁以保护共享资源
    if (_sendMsgList.CanSwap())
    {
        _sendMsgList.Swap(); // 交换待发送的数据列表
    }
    _sendMsgMutex.unlock(); // 解锁

    auto pList = _sendMsgList.GetReaderCache(); // 获取可读数据包列表
    for (auto iter = pList->begin(); iter != pList->end(); ++iter)
    {
        Packet* pPacket = (*iter); // 获取当前数据包
        auto socket = pPacket->GetSocketKey()->Socket; // 获取与数据包关联的 socket
        const auto pObj = _connects[socket]; // 获取连接对象
        if (pObj == nullptr)
        {
            LOG_ERROR("failed to send packet. can't find connect." << pPacket);
            DynamicPacketPool::GetInstance()->FreeObject(pPacket); // 释放数据包对象
            continue;
        }

        // 检查连接对象的关键字是否与数据包相符
        if (!pObj->GetTagKey()->CompareTags(pPacket->GetTagKey()))
        {
            LOG_ERROR("failed to send packet. connect key is different. packet[" << pPacket << "] connect:[" << pObj << "]");
            DynamicPacketPool::GetInstance()->FreeObject(pPacket); // 释放数据包对象
            continue;
        }

        pObj->SendPacket(pPacket); // 发送数据包

#ifdef  EPOLL
        ModifyEvent(_epfd, socket, EPOLLIN | EPOLLOUT | EPOLLRDHUP); // 修改 socket 事件类型
#endif
    }
    pList->clear(); // 清空可读数据包列表
}

// 发送数据包
void Network::SendPacket(Packet*& pPacket)
{
    std::lock_guard<std::mutex> guard(_sendMsgMutex); // 使用智能指针加锁

    // 检查数据包的网络类型是否与当前网络类型一致
    if (pPacket->GetSocketKey()->NetType != _networkType)
    {
        LOG_ERROR("failed to send packet. network type is different." << pPacket);
        return; // 如果类型不匹配，记录错误并返回
    }

    _sendMsgList.GetWriterCache()->emplace_back(pPacket); // 将数据包添加到待发送列表中

#ifdef LOG_TRACE_COMPONENT_OPEN
    const google::protobuf::EnumDescriptor* descriptor = Proto::MsgId_descriptor(); // 获取消息 ID 描述符
    const auto name = descriptor->FindValueByNumber(pPacket->GetMsgId())->name(); // 获取消息 ID 名称

    const auto traceMsg = std::string("send net.")
        .append(" sn:").append(std::to_string(pPacket->GetSN())) // 添加序列号
        .append(" msgId:").append(name); // 添加消息 ID
    ComponentHelp::GetTraceComponent()->Trace(TraceType::Packet, pPacket->GetSocketKey()->Socket, traceMsg); // 记录发送日志
#endif
}
