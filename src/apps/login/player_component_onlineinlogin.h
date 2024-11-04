#pragma once
#include "libserver/message_system.h" // 包含消息系统头文件
#include "libserver/component.h" // 包含组件类头文件

// 该类代表玩家在登录系统中的在线组件，继承自 Component 类和 IAwakeFromPoolSystem 接口
class PlayerComponentOnlineInLogin :public Component<PlayerComponentOnlineInLogin>, public IAwakeFromPoolSystem<std::string>
{
public:
    // 组件唤醒时调用的函数，接收玩家账号作为参数
    void Awake(std::string account) override;

    // 组件回收到对象池时调用的函数，重置内部状态
    void BackToPool() override;

    // 设置玩家在线标志
    void SetOnlineFlag() const;

private:
    // 存储玩家账号
    std::string _account{ "" };
};
