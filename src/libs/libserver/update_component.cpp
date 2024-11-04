#include "update_component.h"

// 唤醒函数的实现
// 当对象从对象池中被提取时，调用此函数并将传入的回调函数 fun 保存到成员变量 _function 中
void UpdateComponent::Awake(UpdateCallBackFun fun)
{
    _function = fun;  // 将传入的回调函数存储到成员变量中
}

// 返回对象池时的清理函数
// 将回调函数清除，避免保留旧的状态或引用导致的内存泄漏或错误行为
void UpdateComponent::BackToPool()
{
    _function = nullptr;  // 将回调函数置为 nullptr，清除引用
}

// 更新函数的实现
// 调用之前设置的回调函数 _function，实现组件的更新操作
void UpdateComponent::Update() const
{
    _function();  // 调用回调函数执行更新逻辑
}
