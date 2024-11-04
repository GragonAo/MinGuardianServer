#pragma once

#include "common.h"

// SnObject 类用于管理唯一的序列号
class SnObject {
public:
    SnObject();  // 构造函数，初始化序列号
    uint64 GetSN() const;  // 获取当前对象的序列号
    void SetSN(uint64 sn);  // 设置对象的序列号

protected:
    uint64 _sn;  // 存储对象的序列号
};
