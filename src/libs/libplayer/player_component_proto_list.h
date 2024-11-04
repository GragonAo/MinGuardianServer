#pragma once

#include "libserver/common.h"
#include "libserver/component.h"
#include "libserver/system.h"

#include <sstream>

// PlayerComponentProtoList 类用于管理玩家列表的协议数据
class PlayerComponentProtoList : public Component<PlayerComponentProtoList>, public IAwakeFromPoolSystem<>
{
public:
    // 初始化方法
    void Awake() override {}

    // 归还到对象池的方法
    void BackToPool() override;

    // 解析玩家列表的协议
    void Parse(Proto::PlayerList& proto);

    // 根据玩家序列号获取协议数据
    std::stringstream* GetProto(uint64 sn);

private:
    // 存储玩家序列号与协议数据的映射
    std::map<uint64, std::stringstream*> _protos;
};
