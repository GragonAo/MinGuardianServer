#include <iostream>
#include <set>

#include "common.h"
#include "network_listen.h"
#include "connect_obj.h"
#include "thread_mgr.h"
#include "network_locator.h"
#include "log4_help.h"
#include "component_help.h"
#include "global.h"
#include "message_system.h"
#include "update_component.h"

// 初始化网络监听器
void NetworkListen::Awake(std::string ip, int port, NetworkType iType)
{
    _networkType = iType; // 设置网络类型

    // 获取网络定位器并将监听器添加到定位器中
    auto pNetworkLocator = ThreadMgr::GetInstance()->GetEntitySystem()->GetComponent<NetworkLocator>();
    pNetworkLocator->AddListenLocator(this, iType);

    // 注册消息处理函数
    auto pMsgSystem = GetSystemManager()->GetMessageSystem();
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_NetworkRequestDisconnect, BindFunP1(this, &NetworkListen::HandleDisconnect));
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_NetworkListenKey, BindFunP1(this, &NetworkListen::HandleListenKey));

    // 添加更新组件
    AddComponent<UpdateComponent>(BindFunP0(this, &NetworkListen::Update));

    // 创建主监听 socket
    _masterSocket = CreateSocket();
    if (_masterSocket == INVALID_SOCKET)
        return; // 如果创建失败，返回

    // 设置 socket 选项，允许地址重用
    int isOn = 1;
    setsockopt(_masterSocket, SOL_SOCKET, SO_REUSEADDR, (SetsockOptType)&isOn, sizeof(isOn));

    // 绑定监听地址和端口
    sockaddr_in addr;
    memset(&addr, 0, sizeof(sockaddr_in)); // 清零结构体
    addr.sin_family = AF_INET; // 设置地址族为 IPv4
    addr.sin_port = htons(port); // 设置端口号
    ::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr.s_addr); // 将 IP 地址从文本转换为网络字节顺序

    // 绑定 socket
    if (::bind(_masterSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
    {
        LOG_ERROR("::bind failed. err:" << _sock_err());
        return; // 如果绑定失败，记录错误并返回
    }

    const int maxConn = 1024; // 最大连接数
    if (::listen(_masterSocket, maxConn) < 0)
    {
        std::cout << "::listen failed." << _sock_err() << std::endl;
        return; // 如果监听失败，记录错误并返回
    }

#ifdef EPOLL
    // 使用 epoll 模式进行监听
    LOG_INFO("epoll model. listen " << ip.c_str() << ":" << port << " SOMAXCONN:" << maxConn);
    InitEpoll(); // 初始化 epoll
    AddEvent(_epfd, _masterSocket, EPOLLIN | EPOLLET | EPOLLOUT | EPOLLRDHUP); // 添加事件
#else
    // 使用 select 模式进行监听
    LOG_INFO("select model. listen " << ip.c_str() << ":" << port << " SOMAXCONN:" << maxConn);
#endif

    return; // 完成初始化
}

// 将网络监听器返回到对象池
void NetworkListen::BackToPool()
{
#ifdef EPOLL
    _mainSocketEventIndex = -1; // 重置主 socket 事件索引
#endif

    // 关闭主监听 socket
    _sock_close(_masterSocket);
    _masterSocket = INVALID_SOCKET; // 设置为无效 socket

    Network::BackToPool(); // 调用基类的返回池方法
}

// 根据应用类型和 ID 初始化网络监听器
void NetworkListen::Awake(int appType, int appId)
{
    auto pGlobal = Global::GetInstance();
    auto pYaml = ComponentHelp::GetYaml();
    const auto pCommonConfig = pYaml->GetIPEndPoint(pGlobal->GetCurAppType(), pGlobal->GetCurAppId());
    if (pCommonConfig == nullptr)
    {
        LOG_ERROR("failed to get config of listen. appType:" << GetAppName(pGlobal->GetCurAppType()) << " appId:" << pGlobal->GetCurAppId());
        return; // 如果未找到配置，记录错误并返回
    }

    // 使用配置初始化网络监听器
    Awake(pCommonConfig->Ip, pCommonConfig->Port, NetworkType::TcpListen);
}

// 使用给定 IP 和端口初始化网络监听器
void NetworkListen::Awake(std::string ip, int port)
{
    Awake(ip, port, NetworkType::HttpListen); // 默认使用 HTTP 监听类型
}

// 接受客户端连接
int NetworkListen::Accept()
{
    struct sockaddr socketClient; // 存储客户端地址
    socklen_t socketLength = sizeof(socketClient); // 客户端地址长度

    int rs = 0; // 记录成功接受的连接数
    while (true)
    {
        const SOCKET socket = ::accept(_masterSocket, &socketClient, &socketLength); // 接受连接
        if (socket == INVALID_SOCKET)
            return rs; // 如果接受失败，返回成功的连接数

        // 如果创建连接对象失败，关闭 socket
        if (!CreateConnectObj(socket, TagType::None, TagValue{}, ConnectStateType::Connected))
        {
            _sock_close(socket);
            continue; // 继续循环等待新的连接
        }

        SetSocketOpt(socket); // 设置新连接的 socket 选项
        ++rs; // 成功接受连接数加一
    }
}

// 获取类的类型名称
const char* NetworkListen::GetTypeName()
{
    return typeid(NetworkListen).name(); // 返回类的类型名称
}

// 获取类的类型哈希码
uint64 NetworkListen::GetTypeHashCode()
{
    return typeid(NetworkListen).hash_code(); // 返回类的哈希码
}

// 显示当前 socket 状态
void NetworkListen::CmdShow()
{
    LOG_DEBUG("\tsocket size:" << _sockets.size()); // 输出当前 socket 数量
}

#ifndef EPOLL

// 更新网络监听器状态
void NetworkListen::Update()
{
#if LOG_TRACE_COMPONENT_OPEN
    CheckBegin(); // 开始跟踪
#endif

    // 清空 fd_set
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);

    // 将主 socket 添加到 fd_set
    FD_SET(_masterSocket, &readfds);
    FD_SET(_masterSocket, &writefds);
    FD_SET(_masterSocket, &exceptfds);

    _fdMax = _masterSocket; // 更新最大 socket 值

    Select(); // 调用 select 函数处理事件

#if LOG_TRACE_COMPONENT_OPEN
    CheckPoint(std::string(GetNetworkTypeName(_networkType)) + "01"); // 记录检查点
#endif

    // 如果主 socket 可读，调用接受函数
    if (FD_ISSET(_masterSocket, &readfds))
    {
        Accept();
    }

    Network::OnNetworkUpdate(); // 更新网络状态

#if LOG_TRACE_COMPONENT_OPEN
    CheckPoint(std::string(GetNetworkTypeName(_networkType)) + "02"); // 记录检查点
#endif
}

#else

// epoll 事件处理
void NetworkListen::OnEpoll(SOCKET fd, int index)
{
    if (fd == _masterSocket) // 如果事件是主 socket
    {
        _mainSocketEventIndex = index; // 更新主 socket 事件索引
    }
}

// 更新网络监听器状态（使用 epoll）
void NetworkListen::Update()
{
    _mainSocketEventIndex = -1; // 重置主 socket 事件索引

    Epoll(); // 调用 epoll 函数处理事件

    // 如果主 socket 有事件，调用接受函数
    if (_mainSocketEventIndex >= 0)
    {
        Accept();
    }

    Network::OnNetworkUpdate(); // 更新网络状态
}

#endif

// 处理网络监听键的消息
void NetworkListen::HandleListenKey(Packet* pPacket)
{
    const auto socketKey = pPacket->GetSocketKey(); // 获取数据包中的 socket 键
    if (socketKey->NetType != _networkType)
        return; // 如果网络类型不匹配，返回

    const auto socket = socketKey->Socket; // 获取 socket
    if (_sockets.find(socket) == _sockets.end()) // 如果 socket 不在集合中
    {
        std::cout << "failed to modify connect key. socket not find." << pPacket << std::endl;
        return; // 输出错误信息并返回
    }

    auto pObj = _connects[socket]; // 获取连接对象
    pObj->GetTagKey()->CopyFrom(pPacket->GetTagKey()); // 修改连接对象的关键字
}
