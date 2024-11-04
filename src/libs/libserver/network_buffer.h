#pragma once

#include "base_buffer.h"

#if TestNetwork
// 测试模式下，发送和接收缓冲区大小设置为 10 字节
#define DEFAULT_SEND_BUFFER_SIZE    10
#define DEFAULT_RECV_BUFFER_SIZE    10
#else
// 默认发送和接收缓冲区大小为 128KB
#define DEFAULT_SEND_BUFFER_SIZE    1024 * 128
#define DEFAULT_RECV_BUFFER_SIZE    1024 * 128
#endif

class Packet;
class ConnectObj;

// 定义总大小的类型，使用 unsigned short 以节省内存
using TotalSizeType = unsigned short;

/**
 * @brief NetworkBuffer 类，继承自 Buffer，表示网络数据缓冲区的基类。
 * 
 * 该类用于管理网络连接中的数据缓冲区，包括数据的存储、读取和写入等操作。
 */
class NetworkBuffer : public Buffer
{
public:
    /**
     * @brief 构造函数，初始化缓冲区大小和关联的网络连接对象。
     * 
     * @param size 缓冲区大小。
     * @param pConnectObj 关联的网络连接对象指针。
     */
    NetworkBuffer(const unsigned int size, ConnectObj* pConnectObj);

    /**
     * @brief 析构函数，释放资源。
     */
    virtual ~NetworkBuffer();

    /**
     * @brief 将缓冲区对象返回到对象池中以重复利用。
     */
    void BackToPool();

    /**
     * @brief 判断缓冲区是否有有效数据。
     * 
     * @return bool 如果有数据返回 true，否则返回 false。
     */
    bool HasData() const;

    /**
     * @brief 获取缓冲区中剩余可写入的空间大小。
     * 
     * @return unsigned int 可写入的字节数。
     */
    unsigned int GetEmptySize() override;

    /**
     * @brief 获取当前写入缓冲区的数据量。
     * 
     * @return unsigned int 写入缓冲区的数据大小。
     */
    unsigned int GetWriteSize() const;

    /**
     * @brief 获取当前可从缓冲区读取的数据量。
     * 
     * @return unsigned int 可读取的数据大小。
     */
    unsigned int GetReadSize() const;

    /**
     * @brief 将缓冲区中的数据填充为指定大小。
     * 
     * @param size 要填充的字节大小。
     */
    void FillDate(unsigned int size);

    /**
     * @brief 从缓冲区移除指定大小的数据。
     * 
     * @param size 要移除的字节大小。
     */
    void RemoveDate(unsigned int size);

    /**
     * @brief 重新分配缓冲区，扩展其容量。
     */
    void ReAllocBuffer();

protected:
    unsigned int _dataSize;  ///< 缓冲区中有效的数据大小。
    ConnectObj* _pConnectObj{ nullptr };  ///< 关联的网络连接对象指针。
};

/**
 * @brief RecvNetworkBuffer 类，继承自 NetworkBuffer，表示接收数据缓冲区。
 * 
 * 该类用于管理从网络连接接收到的数据，并提供解析数据包的功能。
 */
class RecvNetworkBuffer : public NetworkBuffer
{
public:
    /**
     * @brief 构造函数，初始化接收缓冲区大小和关联的网络连接对象。
     * 
     * @param _size 缓冲区大小。
     * @param pConnectObj 关联的网络连接对象指针。
     */
    RecvNetworkBuffer(unsigned int _size, ConnectObj* pConnectObj);

    /**
     * @brief 获取指向缓冲区的指针，用于接收数据。
     * 
     * @param pBuffer 缓冲区指针的引用。
     * @return int 返回缓冲区可用空间的大小。
     */
    int GetBuffer(char*& pBuffer) const;

    /**
     * @brief 从接收缓冲区中获取一个数据包。
     * 
     * @return Packet* 返回解析出的数据包指针。
     */
    Packet* GetPacket();

protected:
    /**
     * @brief 解析 TCP 数据包。
     * 
     * @return Packet* 返回解析出的 TCP 数据包指针。
     */
    Packet* GetTcpPacket();

    /**
     * @brief 解析 HTTP 数据包。
     * 
     * @return Packet* 返回解析出的 HTTP 数据包指针。
     */
    Packet* GetHttpPacket();

private:
    /**
     * @brief 从缓冲区复制数据。
     * 
     * @param pVoid 目标缓冲区指针。
     * @param size 要复制的字节大小。
     */
    void MemcpyFromBuffer(char* pVoid, unsigned int size);
};

/**
 * @brief SendNetworkBuffer 类，继承自 NetworkBuffer，表示发送数据缓冲区。
 * 
 * 该类用于管理待发送的网络数据，并提供打包和发送数据的功能。
 */
class SendNetworkBuffer : public NetworkBuffer
{
public:
    /**
     * @brief 构造函数，初始化发送缓冲区大小和关联的网络连接对象。
     * 
     * @param _size 缓冲区大小。
     * @param pConnectObj 关联的网络连接对象指针。
     */
    SendNetworkBuffer(unsigned int _size, ConnectObj* pConnectObj);

    /**
     * @brief 获取指向缓冲区的指针，用于填充待发送的数据。
     * 
     * @param pBuffer 缓冲区指针的引用。
     * @return int 返回缓冲区可用空间的大小。
     */
    int GetBuffer(char*& pBuffer) const;

    /**
     * @brief 将数据包添加到发送缓冲区中。
     * 
     * @param pPacket 要发送的数据包指针。
     */
    void AddPacket(Packet* pPacket);

private:
    /**
     * @brief 将数据从指定缓冲区复制到发送缓冲区。
     * 
     * @param pVoid 源缓冲区指针。
     * @param size 要复制的字节大小。
     */
    void MemcpyToBuffer(char* pVoid, unsigned int size);
};
