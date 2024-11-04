#pragma once
#include "component.h"
#include "system.h"
#include <functional>

// 定义一个更新回调函数类型，使用std::function封装，便于传递不同类型的函数或可调用对象
using UpdateCallBackFun = std::function<void()>;

// UpdateComponent类，继承自通用组件模板 Component，并实现从对象池唤醒系统 IAwakeFromPoolSystem
class UpdateComponent : public Component<UpdateComponent>, public IAwakeFromPoolSystem<UpdateCallBackFun>
{
public:
    // 唤醒函数，接收一个回调函数作为参数
    // 当组件从对象池中被唤醒时，会执行此函数，将传入的回调函数保存到成员变量 _function 中
    void Awake(UpdateCallBackFun fun) override;

    // 返回对象池时的处理函数
    // 用于清理资源，防止内存泄漏或不必要的状态保留。此处将回调函数置为空
    void BackToPool() override;

    // 更新函数，调用之前设置的回调函数，实现组件的定时更新功能
    void Update() const;

private:
    // 用于存储更新回调函数，当 Update 被调用时会执行此函数
    UpdateCallBackFun _function{ nullptr };
};
