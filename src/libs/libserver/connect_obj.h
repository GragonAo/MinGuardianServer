#pragma once

#include "common.h"             // 引入公共头文件
#include "entity.h"             // 引入实体类定义
#include "system.h"             // 引入系统相关类定义
#include "socket_object.h"      // 引入Socket相关定义

// 前向声明，以避免头文件的循环依赖
class RecvNetworkBuffer;
class SendNetworkBuffer;
class Packet;

// Ping时间间隔（毫秒）
#define PingTime 1000           // 1秒
// Ping延迟时间（毫秒）
#define PingDelayTime  10 * 1000 // 10秒

// 枚举类：表示连接状态类型
enum class ConnectStateType
{
    None,         // 无连接
    Connecting,   // 正在连接
    Connected,    // 已连接
};

// ConnectObj类，继承自Entity和其他系统接口
// ConnectObj表示一个网络连接对象，负责管理接收和发送缓冲区，并维护连接状态
class ConnectObj : public Entity<ConnectObj>,               // 继承Entity模板类，泛型为ConnectObj
                   public NetIdentify,                  // 实现网络标识接口
                   public IAwakeFromPoolSystem<SOCKET, NetworkType, TagType,TagValue, ConnectStateType> // 实现从池中唤醒的系统接口
{
public:
    // 构造函数，初始化连接对象
    ConnectObj();
    // 析构函数，清理资源
    virtual ~ConnectObj();

    // 覆盖IAwakeFromPoolSystem的Awake方法
    // 通过传递socket、网络类型、对象键和连接状态来唤醒对象
    void Awake(SOCKET socket, NetworkType networkType, TagType tagType,TagValue tagValue,ConnectStateType state) override;

    // 覆盖系统接口方法，将对象放回池中
    virtual void BackToPool() override;

    // 检查是否有接收到的数据
    bool HasRecvData() const;
    
    // 处理接收操作，从网络中接收数据
    bool Recv();

    // 检查是否有待发送的数据
    bool HasSendData() const;
    
    // 将Packet数据添加到发送缓冲区中
    void SendPacket(Packet* pPacket) const;
    
    // 处理发送操作，将缓冲区的数据发送出去
    bool Send();
    
    // 关闭连接，释放相关资源
    void Close();
    
    // 获取当前的连接状态
    ConnectStateType GetState() const;
    
    // 将连接状态改为已连接
    void ChangeStateToConnected();


protected:
    ConnectStateType _state{ ConnectStateType::None };   // 当前连接状态，默认初始化为None

    RecvNetworkBuffer* _recvBuffer{ nullptr };           // 接收缓冲区指针，用于处理接收的数据
    SendNetworkBuffer* _sendBuffer{ nullptr };           // 发送缓冲区指针，用于管理待发送的数据
};
