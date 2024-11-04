#pragma once

#include "entity.h"
#include "system.h"
#include "message_system.h"

class Packet;

// 创建组件的系统类，继承自 Entity 和 IAwakeSystem
class CreateComponentC : public Entity<CreateComponentC>, public IAwakeSystem<>
{
public:
    // 初始化函数，重写自 IAwakeSystem
    void Awake() override;

    // 回收函数，重写自 IAwakeSystem
    void BackToPool() override;

private:
    // 处理创建组件的消息
    void HandleCreateComponent(Packet* pPacket) const;

    // 处理移除组件的消息
    void HandleRemoveComponent(Packet* pPacket);

    // 处理创建系统的消息
    void HandleCreateSystem(Packet* pPacket);
};
