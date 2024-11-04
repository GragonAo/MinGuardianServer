#pragma once

#include <map>
#include "common.h"
#include "entity.h"
#include "cache_swap.h"
#include "network_help.h"
#include "connect_obj.h"
#include "trace_component.h"
#include "check_time_component.h"

// 针对非 Windows 平台的宏定义
#if ENGINE_PLATFORM != PLATFORM_WIN32

#define MAX_CLIENT  5120  // 最大客户端连接数

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 

#include <arpa/inet.h>  // 提供网络操作的函数和结构体定义
#include <netinet/in.h> // 提供 IP 地址和端口的结构定义
#include <sys/types.h>  // 基本系统数据类型
#include <sys/socket.h> // 套接字相关函数和结构体
#include <netinet/tcp.h> // TCP/IP 协议

#ifdef EPOLL
#include <sys/epoll.h>  // epoll 系统调用相关头文件
#endif

// 非 Windows 平台下的套接字操作封装
#define _sock_init( )  // 初始化套接字 (Linux 不需要操作)
#define _sock_nonblock( sockfd ) { int flags = fcntl(sockfd, F_GETFL, 0); fcntl(sockfd, F_SETFL, flags | O_NONBLOCK); } // 设置非阻塞模式
#define _sock_exit( )  // 退出套接字 (Linux 不需要操作)
#define _sock_err( ) errno  // 获取套接字错误码
#define _sock_close( sockfd ) ::close( sockfd )  // 关闭套接字
#define _sock_is_blocked()	(errno == EAGAIN || errno == 0)  // 判断是否阻塞

#ifdef EPOLL
// 移除连接对象的宏定义
#define RemoveConnectObj(socket) \
    _connects[socket]->ComponentBackToPool( ); \
    _connects[socket] = nullptr; \
    DeleteEvent(_epfd, socket); \
    _sockets.erase(socket); 
#else
// 移除连接对象的宏定义
#define RemoveConnectObj(socket) \
    _connects[socket]->ComponentBackToPool( ); \
    _connects[socket] = nullptr; \
    _sockets.erase(socket); 
    
#define RemoveConnectObjByItem(iter) \
    _connects[*iter]->ComponentBackToPool(); \
    _connects[*iter] = nullptr; \
    iter = _sockets.erase(iter);
#endif




#else

// 针对 Windows 平台的宏定义
#define MAX_CLIENT  10000  // 最大客户端连接数

#define _sock_init( )	{ WSADATA wsaData; WSAStartup( MAKEWORD(2, 2), &wsaData ); }  // 初始化 Windows 套接字
#define _sock_nonblock( sockfd )	{ unsigned long param = 1; ioctlsocket(sockfd, FIONBIO, (unsigned long *)&param); }  // 设置非阻塞模式
#define _sock_exit( )	{ WSACleanup(); }  // 关闭套接字库
#define _sock_err( )	WSAGetLastError()  // 获取套接字错误码
#define _sock_close( sockfd ) ::closesocket( sockfd )  // 关闭套接字
#define _sock_is_blocked()	(WSAGetLastError() == WSAEWOULDBLOCK)  // 判断是否阻塞

// 移除连接对象的宏定义
#define RemoveConnectObj(socket) \
    _connects[socket]->ComponentBackToPool( ); \
    _connects[socket] = nullptr; \
    _sockets.erase(socket); 
#endif

// 针对不同平台的 setsockopt 类型定义
#if ENGINE_PLATFORM != PLATFORM_WIN32
#define SetsockOptType void *
#else
#define SetsockOptType const char *
#endif

// 前向声明 Packet 类
class Packet;

// Network 类定义，继承 Entity 和 INetwork
class Network : public Entity<Network>, public INetwork
#if LOG_TRACE_COMPONENT_OPEN
    , public CheckTimeComponent  // 如果开启日志跟踪组件，继承 CheckTimeComponent
#endif
{
public:
    // 重置对象池时调用的函数
    void BackToPool() override;
    
    // 发送数据包
    void SendPacket(Packet*& pPacket) override;
    
    // 获取网络类型
    NetworkType GetNetworkType() const { return _networkType; }

protected:
    // 设置套接字选项
    void SetSocketOpt(SOCKET socket);
    
    // 创建套接字
    SOCKET CreateSocket();
    
    // 检查套接字状态
    bool CheckSocket(SOCKET socket);
    
    // 创建连接对象
    bool CreateConnectObj(SOCKET socket, TagType tagType, TagValue tagValue,  ConnectStateType iState);

    // 数据包处理
    void HandleDisconnect(Packet* pPacket);

#ifdef EPOLL
    // epoll 相关方法 (Linux 下的 I/O 多路复用)
    void InitEpoll();
    void Epoll();
    void AddEvent(int epollfd, int fd, int flag);
    void ModifyEvent(int epollfd, int fd, int flag);
    void DeleteEvent(int epollfd, int fd);
    virtual void OnEpoll(SOCKET fd, int index) { };  // 虚拟函数，用于 epoll 事件回调
#else    
    // select 相关方法 (Windows 下的 I/O 多路复用)
    void Select();
#endif

    // 网络更新函数
    void OnNetworkUpdate();

protected:
    // 存储连接对象的数组
    ConnectObj* _connects[MAX_CLIENT]{};
    
    // 记录活动套接字
    std::set<SOCKET> _sockets;

#ifdef EPOLL
    // epoll 使用的事件数组和 epoll 文件描述符
    #define MAX_EVENT   5120
    struct epoll_event _events[MAX_EVENT];
    int _epfd{ -1 };
#else
    // select 使用的文件描述符集
    SOCKET _fdMax{ INVALID_SOCKET };
    fd_set readfds, writefds, exceptfds;
#endif

    // 锁和缓存交换，用于发送消息
    std::mutex _sendMsgMutex;
    CacheSwap<Packet> _sendMsgList;

    // 网络类型（默认为 TCP 监听）
    NetworkType _networkType{ NetworkType::TcpListen };
};
