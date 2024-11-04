#pragma once
#include "libserver/system.h"
#include "libserver/component.h"
#include "libplayer/player_component.h"

// 玩家在游戏中的在线状态组件
class PlayerComponentOnlineInGame : public Component<PlayerComponentOnlineInGame>,
    public PlayerComponent,  // 继承玩家组件
    virtual public IAwakeFromPoolSystem<std::string, int>,  // 从对象池中唤醒（使用账号和版本）
    virtual public IAwakeFromPoolSystem<std::string>  // 从对象池中唤醒（仅使用账号）
{
public:
    // 重载的唤醒方法，使用账号和版本进行初始化
    void Awake(std::string account, int version) override;

    // 重载的唤醒方法，使用账号进行初始化
    void Awake(std::string account) override;

    // 将当前对象归还到对象池并重置状态
    void BackToPool() override;

    // 设置玩家的在线标志
    void SetOnlineFlag() const;

    // 从 Proto 数据解析玩家信息
    void ParserFromProto(const Proto::Player& proto) override;

    // 序列化玩家信息到 Proto 数据中
    void SerializeToProto(Proto::Player* pProto) override;

private:
    // 玩家账号
    std::string _account{ "" };

    // 在线版本号
    int _onlineVersion{ 0 };

    // 标志位，指示是否从数据库读取
    bool _isReadFromeDB{ true };
};
