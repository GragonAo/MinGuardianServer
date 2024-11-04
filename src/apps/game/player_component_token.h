#pragma once
#include "libserver/component.h"
#include "libserver/system.h"

// 玩家组件：用于处理玩家的 Token
class PlayerComponentToken : public Component<PlayerComponentToken>, public IAwakeFromPoolSystem<std::string>
{
public:
    // 当组件从对象池中被激活时调用，初始化 Token
    void Awake(const std::string token) override;

    // 当组件被回收到对象池时调用，重置或清理数据
    void BackToPool() override;

    // 检查传入的 Token 是否有效
    bool IsTokenValid(std::string token) const;

private:
    // 存储玩家的 Token
    std::string _token;
};
