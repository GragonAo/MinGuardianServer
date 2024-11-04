#include "base_buffer.h"
#include "log4_help.h"

#include <iostream>
#include <cstring>

// 获取当前空闲的缓冲区大小
unsigned Buffer::GetEmptySize()
{
    // 返回缓冲区总大小减去当前结束索引，表示剩余可用空间
    return _bufferSize - _endIndex;
}

// 重新分配缓冲区以适应新的数据长度
void Buffer::ReAllocBuffer(const unsigned int dataLength)
{
    // 检查当前缓冲区是否已经达到最大大小
    if (_bufferSize >= MAX_SIZE) {
        std::cout << "Buffer::Realloc except!! Max size:" << _bufferSize << std::endl;
        return; // 达到最大大小，返回，不进行重新分配
    }

    // 创建一个新的缓冲区，大小为当前缓冲区大小加上额外的大小
    char* tempBuffer = new char[_bufferSize + ADDITIONAL_SIZE];
    unsigned int _newEndIndex; // 新的结束索引

    // 检查开始索引与结束索引的关系，以决定如何复制数据
    if (_beginIndex < _endIndex)
    {
        // 情况 1：缓冲区中有有效数据，直接从旧缓冲区复制数据
        ::memcpy(tempBuffer, _buffer + _beginIndex, _endIndex - _beginIndex);
        _newEndIndex = _endIndex - _beginIndex; // 更新新的结束索引
    }
    else
    {
        // 情况 2：缓冲区是循环使用的
        if (_beginIndex == _endIndex && dataLength <= 0)
        {
            // 如果开始和结束索引相同，并且没有新数据，新的结束索引为 0
            _newEndIndex = 0;
        }
        else 
        {
            // 1. 复制缓冲区尾部的数据
            ::memcpy(tempBuffer, _buffer + _beginIndex, _bufferSize - _beginIndex);
            _newEndIndex = _bufferSize - _beginIndex; // 更新新的结束索引

            // 2. 复制缓冲区头部的数据（如果有）
            if (_endIndex > 0)
            {
                ::memcpy(tempBuffer + _newEndIndex, _buffer, _endIndex);
                _newEndIndex += _endIndex; // 更新新的结束索引
            }
        }
    }

    // 更新缓冲区大小，增加额外的大小
    _bufferSize += ADDITIONAL_SIZE;

    // 释放旧的缓冲区内存
    delete[] _buffer;
    // 将缓冲区指针指向新的缓冲区
    _buffer = tempBuffer;

    // 更新开始索引和结束索引
    _beginIndex = 0;
    _endIndex = _newEndIndex;

    // 在测试网络模式下记录日志
#if TestNetwork
    LOG_WARN("Buffer::Realloc. _bufferSize:" << _bufferSize);
#endif
}
