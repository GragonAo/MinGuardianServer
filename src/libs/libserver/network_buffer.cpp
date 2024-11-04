#include "libserver/socket_object.h"
#include "packet.h"
#include "network_buffer.h"
#include "connect_obj.h"
#include "message_system_help.h"
#include "network.h"

#include <cstdlib>
#include <cstring>
#include "mongoose/mongoose.h"

// NetworkBuffer类构造函数
NetworkBuffer::NetworkBuffer(const unsigned int size, ConnectObj* pConnectObj)
{
    _pConnectObj = pConnectObj; // 关联连接对象
    _bufferSize = size;          // 设置缓冲区大小
    _beginIndex = 0;             // 初始化读取位置
    _endIndex = 0;               // 初始化写入位置
    _dataSize = 0;               // 初始化有效数据大小
    _buffer = new char[_bufferSize]; // 动态分配缓冲区
}

// NetworkBuffer类析构函数
NetworkBuffer::~NetworkBuffer()
{
    delete[] _buffer; // 释放动态分配的缓冲区
}

// 重置缓冲区状态，用于复用
void NetworkBuffer::BackToPool()
{
    _beginIndex = 0; // 重置读取位置
    _endIndex = 0;   // 重置写入位置
    _dataSize = 0;   // 重置有效数据大小
}

// 检查缓冲区是否有有效数据
bool NetworkBuffer::HasData() const
{
    if (_dataSize <= 0)
        return false; // 如果有效数据大小小于等于0，返回false

    // 需要确保有足够的数据以读取一个Packet头
    if (_dataSize < sizeof(PacketHead))
        return false;

    return true; // 否则返回true
}

// 获取缓冲区中的剩余可写入空间大小
unsigned int NetworkBuffer::GetEmptySize()
{
    return _bufferSize - _dataSize; // 返回剩余空间大小
}

// 获取当前可以写入的字节数
unsigned int NetworkBuffer::GetWriteSize() const
{
    if (_beginIndex <= _endIndex)
    {
        return _bufferSize - _endIndex; // 计算写入位置到缓冲区末尾的字节数
    }
    else
    {
        return _beginIndex - _endIndex; // 计算写入位置到读取位置的字节数
    }
}

// 获取当前可读取的字节数
unsigned int NetworkBuffer::GetReadSize() const
{
    if (_dataSize <= 0)
        return 0; // 如果没有有效数据，返回0

    if (_beginIndex < _endIndex)
    {
        return _endIndex - _beginIndex; // 计算读取位置到写入位置的字节数
    }
    else
    {
        return _bufferSize - _beginIndex; // 计算读取位置到缓冲区末尾的字节数
    }
}

// 在缓冲区中填充指定大小的数据
void NetworkBuffer::FillDate(unsigned int size)
{
    _dataSize += size; // 增加有效数据大小

    // 如果填充后超过了缓冲区末尾，更新写入位置
    if ((_bufferSize - _endIndex) <= size)
    {
        size -= _bufferSize - _endIndex; // 更新需要填充的剩余大小
        _endIndex = 0; // 重置写入位置
    }

    _endIndex += size; // 更新写入位置
}

// 移除缓冲区中指定大小的数据
void NetworkBuffer::RemoveDate(unsigned int size)
{
    _dataSize -= size; // 减少有效数据大小

    // 如果移除后读取位置超过了缓冲区末尾，更新读取位置
    if ((_beginIndex + size) >= _bufferSize)
    {
        size -= _bufferSize - _beginIndex; // 更新需要移除的剩余大小
        _beginIndex = 0; // 重置读取位置
    }

    _beginIndex += size; // 更新读取位置
}

// 重新分配缓冲区以扩大其容量
void NetworkBuffer::ReAllocBuffer()
{
    Buffer::ReAllocBuffer(_dataSize); // 调用基类方法重新分配缓冲区
}

///////////////////////////////////////////////////////////////////////////////////////////////

// RecvNetworkBuffer类构造函数
RecvNetworkBuffer::RecvNetworkBuffer(const unsigned int size, ConnectObj* pConnectObj) 
    : NetworkBuffer(size, pConnectObj) 
{
}

// 获取缓冲区的当前写入位置
int RecvNetworkBuffer::GetBuffer(char*& pBuffer) const
{
    pBuffer = _buffer + _endIndex; // 设置pBuffer为当前写入位置
    return GetWriteSize(); // 返回可写入空间的大小
}

// 从缓冲区获取数据包，支持TCP和HTTP类型
Packet* RecvNetworkBuffer::GetPacket()
{
    auto pNetwork = _pConnectObj->GetParent<Network>();
    if (!NetworkHelp::IsTcp(pNetwork->GetNetworkType()))
        return GetHttpPacket(); // 不是TCP类型，尝试解析HTTP包

    return GetTcpPacket(); // 否则解析TCP包
}

// 从缓冲区获取TCP数据包
Packet* RecvNetworkBuffer::GetTcpPacket()
{
    // 检查缓冲区中是否有足够的数据以读取总大小
    if (_dataSize < sizeof(TotalSizeType))
    {
        return nullptr; // 如果数据不足，返回空
    }

    // 1. 读取数据包总大小
    unsigned short totalSize;
    MemcpyFromBuffer(reinterpret_cast<char*>(&totalSize), sizeof(TotalSizeType));

    // 检查有效数据大小是否足够
    if (_dataSize < totalSize)
    {
        return nullptr; // 数据不足，返回空
    }

    RemoveDate(sizeof(TotalSizeType)); // 移除总大小数据

 // 2.读取头部大小
    unsigned short headSize;
    MemcpyFromBuffer(reinterpret_cast<char*>(&headSize), sizeof(TotalSizeType));
    RemoveDate(sizeof(TotalSizeType));

    // 3.解析 PacketHead
    PacketHead* pHead;
    PacketHeadS2S* pHeadS2s = nullptr;
    if (headSize == sizeof(PacketHead)) 
    {
        PacketHead head;
        MemcpyFromBuffer(reinterpret_cast<char*>(&head), sizeof(PacketHead));
        RemoveDate(sizeof(PacketHead));
        pHead = &head;
    }
    else
    {
        PacketHeadS2S head;
        MemcpyFromBuffer(reinterpret_cast<char*>(&head), sizeof(PacketHeadS2S));
        RemoveDate(sizeof(PacketHeadS2S));
        pHead = &head;
        pHeadS2s = &head;
    }


    // 3. 检查消息ID的有效性
    const google::protobuf::EnumDescriptor* descriptor = Proto::MsgId_descriptor();
    if (descriptor->FindValueByNumber(pHead->MsgId) == nullptr)
    {
        // 无效消息ID，关闭连接
        _pConnectObj->Close();
        std::cout << "recv invalid msg." << std::endl;
        return nullptr; // 返回空
    }

    // 根据消息ID创建数据包
    Packet* pPacket = MessageSystemHelp::CreatePacket((Proto::MsgId)pHead->MsgId, _pConnectObj);
    unsigned int dataLength = totalSize - sizeof(PacketHead) - sizeof(TotalSizeType) * 2;
    if (pHeadS2s != nullptr)
        dataLength = totalSize - sizeof(PacketHeadS2S) - sizeof(TotalSizeType) * 2;

    // 确保数据包有足够的空间存储数据
    while (pPacket->GetTotalSize() < dataLength)
    {
        pPacket->ReAllocBuffer();
    }

    // 从缓冲区复制数据到数据包
    MemcpyFromBuffer(pPacket->GetBuffer(), dataLength);
    pPacket->FillData(dataLength); // 更新数据包的有效数据大小
    RemoveDate(dataLength); // 移除数据
    
    if (pHeadS2s != nullptr)
    {
        auto pTagKey = pPacket->GetTagKey();
        pTagKey->AddTag(TagType::Entity, pHeadS2s->EntitySn);
        pTagKey->AddTag(TagType::Player, pHeadS2s->PlayerSn);
    }

    return pPacket; // 返回解析后的数据包
}
// Packet* RecvNetworkBuffer::GetHttpPacket()
// {
//     // 检查读取索引是否超出写入索引，表示数据异常
//     if (_endIndex < _beginIndex)
//     {
//         _pConnectObj->Close(); // 关闭连接
//         LOG_ERROR("http recv invalid."); // 记录错误日志
//         return nullptr; // 返回空指针
//     }

//     // 计算可读取的字节数
//     const unsigned int recvBufLength = _endIndex - _beginIndex;
//     const auto pNetwork = _pConnectObj->GetParent<Network>(); // 获取父网络对象
//     const auto iType = pNetwork->GetNetworkType(); // 获取网络类型
//     const bool isConnector = iType == NetworkType::HttpConnector; // 判断是否为HTTP连接器

//     http_message hm; // 存储HTTP消息
//     // 解析HTTP头部并获取头部长度
//     const unsigned int headerLen = mg_parse_http(_buffer + _beginIndex, _endIndex - _beginIndex, &hm, !isConnector);
//     if (headerLen <= 0)
//         return nullptr; // 如果头部长度无效，返回空

//     unsigned int bodyLen = 0; // 初始化请求体长度
//     const auto mgBody = mg_get_http_header(&hm, "Content-Length"); // 获取内容长度
//     if (mgBody != nullptr)
//     {
//         bodyLen = atoi(mgBody->p); // 将内容长度转换为整数

//         // 检查可读取的长度是否足以容纳请求体和头部
//         if (bodyLen > 0 && (recvBufLength < (bodyLen + headerLen)))
//             return nullptr; // 数据不足，返回空
//     }

//     bool isChunked = false; // 标记是否为chunked传输编码
//     const auto mgTransferEncoding = mg_get_http_header(&hm, "Transfer-Encoding");
//     if (mgTransferEncoding != nullptr && mg_vcasecmp(mgTransferEncoding, "chunked") == 0)
//     {
//         isChunked = true; // 设置为chunked传输编码

//         // 如果接收到的数据长度等于请求头长度，返回空
//         if (recvBufLength == headerLen)
//             return nullptr;

//         // 获取请求体长度
//         bodyLen = mg_http_get_request_len(_buffer + _beginIndex + headerLen, recvBufLength - headerLen);
//         if (bodyLen <= 0)
//             return nullptr; // 返回空

//         bodyLen = _endIndex - _beginIndex - headerLen; // 更新请求体长度
//     }

// #ifdef LOG_HTTP_OPEN
//     // 如果开启HTTP日志，记录请求数据
//     std::stringstream allBuffer;
//     allBuffer.write(_buffer + _beginIndex, (bodyLen + headerLen)); // 记录头部和请求体数据
//     LOG_HTTP("\r\n" << allBuffer.str().c_str()); // 输出日志
// #endif

//     // 解析HTTP数据并创建Packet
//     Packet* pPacket = MessageSystemHelp::ParseHttp(_pConnectObj, _buffer + _beginIndex + headerLen, bodyLen, isChunked, &hm);
//     RemoveDate(bodyLen + headerLen); // 移除已处理的数据
//     return pPacket; // 返回解析后的Packet
// }

Packet* RecvNetworkBuffer::GetHttpPacket()
{
    // 检查读取索引是否超出写入索引，表示数据异常
    if (_endIndex < _beginIndex)
    {
        _pConnectObj->Close(); // 关闭连接
        LOG_ERROR("http recv invalid."); // 记录错误日志
        return nullptr; // 返回空指针
    }

    // 计算接收到的字节数
    const unsigned int recvBufLength = _endIndex - _beginIndex;
    const auto pNetwork = _pConnectObj->GetParent<Network>(); // 获取父网络对象
    const auto iType = pNetwork->GetNetworkType(); // 获取网络类型
    const bool isConnector = iType == NetworkType::HttpConnector; // 判断是否为HTTP连接器

    http_message hm; // 存储HTTP消息
    // 解析HTTP头部并获取头部长度
    const unsigned int headerLen = mg_parse_http(_buffer + _beginIndex, recvBufLength, &hm, !isConnector);
    if (headerLen <= 0)
        return nullptr; // 如果头部长度无效，返回空

    unsigned int bodyLen = 0; // 初始化请求体长度
    const auto mgBody = mg_get_http_header(&hm, "Content-Length"); // 尝试获取Content-Length头

    if (mgBody != nullptr)
    {
        // 使用Content-Length确定body的长度
        bodyLen = atoi(mgBody->p); // 将内容长度转换为整数

        // 如果接收缓冲区中数据不足，等待更多数据
        if (bodyLen > 0 && (recvBufLength < (bodyLen + headerLen)))
            return nullptr;
    }

    bool isChunked = false; // 标记是否为chunked传输编码
    const auto mgTransferEncoding = mg_get_http_header(&hm, "Transfer-Encoding");
    if (mgTransferEncoding != nullptr && mg_vcasecmp(mgTransferEncoding, "chunked") == 0)
    {
        // 如果传输编码是chunked，设置isChunked标志
        isChunked = true;

        // 如果接收到的数据长度等于请求头长度，表示无请求体数据，返回空
        if (recvBufLength == headerLen)
            return nullptr;

        // 设置bodyLen为解析后的chunked数据长度
        bodyLen = recvBufLength - headerLen;
    }

#ifdef LOG_HTTP_OPEN
    // 如果开启HTTP日志，记录整个请求数据（包括头部和body）
    std::stringstream allBuffer;
    allBuffer.write(_buffer + _beginIndex, (bodyLen + headerLen));
    LOG_HTTP("\r\n" << allBuffer.str().c_str()); // 输出日志
#endif

    // 解析HTTP数据并创建Packet
    Packet* pPacket = MessageSystemHelp::ParseHttp(_pConnectObj, _buffer + _beginIndex + headerLen, bodyLen, isChunked, &hm);
    // 移除已经处理的数据（header + body长度）
    RemoveDate(bodyLen + headerLen);

    return pPacket; // 返回解析后的Packet
}


void RecvNetworkBuffer::MemcpyFromBuffer(char* pVoid, const unsigned int size)
{
    const auto readSize = GetReadSize(); // 获取当前可读取的字节数
    if (readSize < size) // 如果可读取字节数小于要复制的字节数
    {
        // 1. 先复制到缓冲区末尾
        ::memcpy(pVoid, _buffer + _beginIndex, readSize); 

        // 2. 继续复制到缓冲区开头
        ::memcpy(pVoid + readSize, _buffer, size - readSize); 
    }
    else // 如果可读取字节数足够
    {
        ::memcpy(pVoid, _buffer + _beginIndex, size); // 直接复制
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////

// SendNetworkBuffer类构造函数
SendNetworkBuffer::SendNetworkBuffer(const unsigned int size, ConnectObj* pConnectObj) 
    : NetworkBuffer(size, pConnectObj) // 调用基类构造函数
{
}

// 获取当前可写入的缓冲区
int SendNetworkBuffer::GetBuffer(char*& pBuffer) const
{
    if (_dataSize <= 0) // 如果没有有效数据
    {
        pBuffer = nullptr; // 设置为null
        return 0; // 返回0
    }

    if (_beginIndex < _endIndex) // 如果写入位置小于读取位置
    {
        pBuffer = _buffer + _beginIndex; // 设置为当前写入位置
        return _endIndex - _beginIndex; // 返回可写入的字节数
    }
    else // 否则
    {
        pBuffer = _buffer + _beginIndex; // 设置为当前写入位置
        return _bufferSize - _beginIndex; // 返回到缓冲区末尾的字节数
    }
}

// 将Packet添加到发送缓冲区
void SendNetworkBuffer::AddPacket(Packet* pPacket)
{
    const auto dataLength = pPacket->GetDataLength(); // 获取数据长度
    const auto pTagValue = pPacket->GetTagKey()->GetTagValue(TagType::Entity);

    TotalSizeType totalSize = dataLength + sizeof(PacketHead) + sizeof(TotalSizeType)*2; // 计算总大小
    if(pTagValue != nullptr){
        totalSize = dataLength + sizeof(PacketHeadS2S) + sizeof(TotalSizeType) * 2;
    }
    // 确保缓冲区有足够的空间
    while (GetEmptySize() < totalSize) {
        ReAllocBuffer(); // 重新分配缓冲区
        //std::cout << "send buffer::Realloc. _bufferSize:" << _bufferSize << std::endl;
    }

    // 对于HTTP协议，body不会有独立的头部
    const auto msgId = pPacket->GetMsgId(); // 获取消息ID
    if (!NetworkHelp::IsHttpMsg(msgId))
    {
        // 1. 写入总大小
        MemcpyToBuffer(reinterpret_cast<char*>(&totalSize), sizeof(TotalSizeType));

        // 2. 写入Packet头
        if (pTagValue == nullptr)
        {
            PacketHead head{};
            head.MsgId = pPacket->GetMsgId();

            TotalSizeType headSize = sizeof(PacketHead);
            MemcpyToBuffer(reinterpret_cast<char*>(&headSize), sizeof(TotalSizeType));
            MemcpyToBuffer(reinterpret_cast<char*>(&head), sizeof(PacketHead));
        }
        else
        {
            PacketHeadS2S head{};
            head.MsgId = pPacket->GetMsgId();

            head.EntitySn = pTagValue->KeyInt64;
            const auto pTagPlayer = pPacket->GetTagKey()->GetTagValue(TagType::Player);
            if (pTagPlayer == nullptr)
                head.PlayerSn = 0;
            else
                head.PlayerSn = pTagPlayer->KeyInt64;

            TotalSizeType headSize = sizeof(PacketHeadS2S);
            MemcpyToBuffer(reinterpret_cast<char*>(&headSize), sizeof(TotalSizeType));
            MemcpyToBuffer(reinterpret_cast<char*>(&head), sizeof(PacketHeadS2S));
        }
    }

    // 3. 写入Packet数据
    MemcpyToBuffer(pPacket->GetBuffer(), pPacket->GetDataLength());
}

// 将数据复制到缓冲区
void SendNetworkBuffer::MemcpyToBuffer(char* pVoid, const unsigned int size)
{
    const auto writeSize = GetWriteSize(); // 获取当前可写入的字节数
    if (writeSize < size) // 如果可写入字节数小于要复制的字节数
    {
        // 1. 先复制到缓冲区末尾
        ::memcpy(_buffer + _endIndex, pVoid, writeSize);

        // 2. 继续复制到缓冲区开头
        ::memcpy(_buffer, pVoid + writeSize, size - writeSize);
    }
    else // 如果可写入字节数足够
    {
        ::memcpy(_buffer + _endIndex, pVoid, size); // 直接复制
    }

    FillDate(size); // 更新有效数据大小
}
