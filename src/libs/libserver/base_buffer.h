#pragma once

// 根据是否启用测试网络，定义额外的缓冲区大小
#if TestNetwork
#define ADDITIONAL_SIZE 10 // 测试网络时，额外大小为 10
#else
#define ADDITIONAL_SIZE 1024 * 128 // 非测试网络时，额外大小为 128KB
#endif

// 定义缓冲区的最大大小为 1MB
#define MAX_SIZE		1024 * 1024 // 1MB

class Buffer
{
public:
    // 获取当前空闲的缓冲区大小
    virtual unsigned int GetEmptySize();

    // 重新分配缓冲区以适应新的数据长度
    void ReAllocBuffer(unsigned int dataLength);

    // 获取缓冲区的结束索引
    unsigned int GetEndIndex() const
    {
        return _endIndex;
    }

    // 获取缓冲区的开始索引
    unsigned int GetBeginIndex() const
    {
        return _beginIndex;
    }

    // 获取缓冲区的总大小
    unsigned int GetTotalSize() const
    {
        return _bufferSize;
    }

protected:
    char* _buffer{ nullptr }; // 指向缓冲区的指针，初始为空
    unsigned int _beginIndex{ 0 }; // 缓冲区的开始索引，初始为 0
    unsigned int _endIndex{ 0 }; // 缓冲区的结束索引，初始为 0

    unsigned int _bufferSize{ 0 }; // 缓冲区的总大小，初始为 0
};
