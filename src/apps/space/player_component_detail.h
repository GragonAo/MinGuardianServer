#pragma once // 确保该头文件只被包含一次

#include "libserver/component.h"          // 包含组件基类的头文件
#include "libserver/system.h"             // 包含系统基类的头文件
#include "libplayer/player_component.h"    // 包含玩家组件的头文件

// PlayerComponentDetail 类声明，继承自多个基类
class PlayerComponentDetail : public Component<PlayerComponentDetail>, 
                              public IAwakeFromPoolSystem<>, 
                              public PlayerComponent
{
public:
    void Awake() override; // 初始化时调用的方法，重写自基类
    void BackToPool() override; // 将组件返回到池的方法，重写自基类

    // 从 Proto 对象解析数据
    void ParserFromProto(const Proto::Player& proto) override; 
    // 将数据序列化到 Proto 对象中
    void SerializeToProto(Proto::Player* pProto) override;

    // 获取玩家的性别
    Proto::Gender GetGender() const;

private:
    Proto::Gender _gender; // 存储玩家性别的私有成员变量
};
