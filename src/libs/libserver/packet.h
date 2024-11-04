#pragma once

#include "base_buffer.h"
#include "common.h"
#include "entity.h"
#include "system.h"
#include "socket_object.h"

// 结构体打包控制，指定对齐方式为4字节
#pragma pack(push)
#pragma pack(4)

// 数据包头部结构
struct PacketHead {
    unsigned short MsgId; // 消息 ID
};

// 服务端到服务端的数据包头部结构
struct PacketHeadS2S : public PacketHead
{
    uint64 EntitySn; // 实体序号
    uint64 PlayerSn;  // 玩家序号
};

#pragma pack(pop)

// 根据测试标志定义默认缓冲区大小
#if TestNetwork
#define DEFAULT_PACKET_BUFFER_SIZE 10 // 测试时使用的缓冲区大小
#else
// 默认大小 10KB
#define DEFAULT_PACKET_BUFFER_SIZE 1024 * 10
#endif

// Packet 类定义
class Packet : public Entity<Packet>, public Buffer, public NetIdentify, public IAwakeFromPoolSystem<Proto::MsgId, NetIdentify*>
{
public:
    Packet(); // 构造函数
    ~Packet(); // 析构函数
    void Awake(Proto::MsgId msgId, NetIdentify* pIdentify) override; // 初始化函数

    // 从缓冲区解析为 ProtoClass 类型的协议对象
    template<class ProtoClass>
    ProtoClass ParseToProto()
    {
        ProtoClass proto;
        proto.ParsePartialFromArray(GetBuffer(), GetDataLength()); // 解析数据
        return proto; // 返回解析后的协议对象
    }

    // 序列化 ProtoClass 类型的协议对象到缓冲区
    template<class ProtoClass>
    void SerializeToBuffer(ProtoClass& protoClase)
    {
        auto total = (unsigned int)protoClase.ByteSizeLong(); // 获取协议对象的字节大小
        while (GetEmptySize() < total) // 如果空闲空间不足
        {
            ReAllocBuffer(); // 重新分配缓冲区
        }

        protoClase.SerializePartialToArray(GetBuffer(), total); // 序列化到缓冲区
        FillData(total); // 填充数据长度
    }

    // 序列化 C 字符串到缓冲区
    void SerializeToBuffer(const char* s, unsigned int len)
    {
        while (GetEmptySize() < len) // 如果空闲空间不足
        {
            ReAllocBuffer(); // 重新分配缓冲区
        }

        ::memcpy(_buffer + _endIndex, s, len); // 拷贝数据到缓冲区
        FillData(len); // 填充数据长度
    }

    void BackToPool() override; // 返回到池
    void CopyFrom(Packet* pPacket); // 从另一个数据包复制内容

    char* GetBuffer() const; // 获取缓冲区指针
    unsigned short GetDataLength() const; // 获取数据长度
    int GetMsgId() const; // 获取消息 ID
    void FillData(unsigned int size); // 填充数据长度
    void ReAllocBuffer(); // 重新分配缓冲区

    // 引用管理
    void AddRef(); // 增加引用计数
    void RemoveRef(); // 减少引用计数
    void OpenRef(); // 打开引用
    bool CanBack2Pool() const; // 检查是否可以返回池

private:
    Proto::MsgId _msgId; // 消息 ID

private:
    std::atomic<int> _ref{ 0 }; // 原子引用计数
    bool _isRefOpen{ false }; // 引用是否开放
};
