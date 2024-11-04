#include "sn_object.h"
#include "global.h"

// SnObject 类构造函数
SnObject::SnObject() {
    _sn = Global::GetInstance()->GenerateSN();  // 生成唯一的序列号
}

// 获取序列号
uint64 SnObject::GetSN() const {
    return _sn;  // 返回当前对象的序列号
}

// 设置序列号
void SnObject::SetSN(uint64 sn) {
    _sn = sn;  // 更新对象的序列号
}
