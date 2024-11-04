#include "packet.h"

// 默认构造函数
Packet::Packet()
{
    _bufferSize = DEFAULT_PACKET_BUFFER_SIZE; // 初始化缓冲区大小
    _beginIndex = 0; // 数据开始索引
    _endIndex = 0;   // 数据结束索引
    _buffer = new char[_bufferSize]; // 分配缓冲区

    _ref = 0; // 引用计数
    _isRefOpen = false; // 引用是否开放
    _socketKey.Clear(); // 清空 socket 键
    _tagKey.Clear();    // 清空标签键
}

// 析构函数
Packet::~Packet()
{
    delete[] _buffer; // 释放缓冲区
}

// 初始化数据包
void Packet::Awake(Proto::MsgId msgId, NetIdentify* pIdentify)
{
    if (pIdentify != nullptr)
    {
        _socketKey.Clear(); // 清空 socket 键
        _tagKey.Clear();    // 清空标签键

        // 从 pIdentify 复制 socket 和标签键
        _socketKey.CopyFrom(pIdentify->GetSocketKey());
        _tagKey.CopyFrom(pIdentify->GetTagKey());
    }
    else
    {
        _socketKey.Clear(); // 清空 socket 键
        _tagKey.Clear();    // 清空标签键
    }

    _msgId = msgId; // 设置消息 ID

    _beginIndex = 0; // 重置开始索引
    _endIndex = 0;   // 重置结束索引
    _ref = 0; // 重置引用计数
    _isRefOpen = false; // 重置引用开放状态
}

// 返回池
void Packet::BackToPool()
{
    _msgId = Proto::MsgId::None; // 清空消息 ID
    _socketKey.Clear(); // 清空 socket 键
    _tagKey.Clear(); // 清空标签键

    _beginIndex = 0; // 重置开始索引
    _endIndex = 0;   // 重置结束索引
    _ref = 0; // 重置引用计数
    _isRefOpen = false; // 重置引用开放状态
}

// 复制数据包内容
void Packet::CopyFrom(Packet* pPacket)
{
    const auto total = pPacket->GetDataLength(); // 获取数据长度
    while (GetEmptySize() < total) // 如果空闲空间不足
    {
        ReAllocBuffer(); // 重新分配缓冲区
    }

    _beginIndex = 0; // 设置开始索引
    _endIndex = total; // 设置结束索引
    memcpy(_buffer, pPacket->GetBuffer(), _endIndex); // 复制数据
}

// 获取缓冲区指针
char* Packet::GetBuffer() const
{
    return _buffer; // 返回缓冲区指针
}

// 获取数据长度
unsigned short Packet::GetDataLength() const
{
    return _endIndex - _beginIndex; // 计算数据长度
}

// 获取消息 ID
int Packet::GetMsgId() const
{
    return _msgId; // 返回消息 ID
}

// 填充数据
void Packet::FillData(const unsigned int size)
{
    _endIndex += size; // 增加结束索引
}

// 重新分配缓冲区
void Packet::ReAllocBuffer()
{
    Buffer::ReAllocBuffer(_endIndex - _beginIndex); // 调用缓冲区重新分配
}

// 增加引用计数
void Packet::AddRef()
{
    ++_ref; // 引用计数加一
}

// 减少引用计数
void Packet::RemoveRef()
{
    --_ref; // 引用计数减一
    if (_ref < 0) // 如果引用计数小于零
    {
        const google::protobuf::EnumDescriptor* descriptor = Proto::MsgId_descriptor();
        const auto name = descriptor->FindValueByNumber(_msgId)->name(); // 获取消息 ID 名称
        LOG_ERROR("packet ref < 0. ref:" << _ref << " msgId:" << name.c_str()); // 记录错误
    }
}

// 打开引用
void Packet::OpenRef()
{
    _isRefOpen = true; // 设置引用开放状态
}

// 检查是否可以返回池
bool Packet::CanBack2Pool() const
{
    if (!_isRefOpen) // 如果引用未开放
        return false;

    if (_ref == 0) // 如果引用计数为零
        return true;

    return false; // 否则不能返回池
}
