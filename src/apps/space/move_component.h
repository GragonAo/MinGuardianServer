#pragma once // 确保头文件只被包含一次

#include "libserver/component.h"                // 包含组件基类的定义
#include "libserver/system.h"                   // 包含系统基类的定义
#include "libserver/vector3.h"                  // 包含 Vector3 类的定义

#include <queue>                                // 包含队列容器的定义

class PlayerComponentLastMap; // 前向声明，避免循环依赖

// 用于存储移动向量的结构
struct MoveVector3
{
    // 更新目标位置和当前位置的移动向量
    void Update(Vector3 target, Vector3 position);

    Vector3 Target = Vector3::Zero; // 当前目标位置，默认为零向量
    float ScaleX{ 0 }; // 在X轴上的移动比例
    float ScaleZ{ 0 }; // 在Z轴上的移动比例
};

// 处理玩家移动的组件
class MoveComponent : public Component<MoveComponent>, public IAwakeFromPoolSystem<>
{
public:
    // 重写 Awake 方法，用于初始化组件
    void Awake() override;

    // 重写 BackToPool 方法，用于将组件返回对象池
    void BackToPool() override;

    // 更新目标队列和当前位置
    void Update(std::queue<Vector3> targets, Vector3 curPosition);

    // 根据经过的时间、最后地图组件和速度更新位置
    bool Update(float timeElapsed, PlayerComponentLastMap* pLastMap, const float speed);

private:
    std::queue<Vector3> _targets; // 存储目标位置的队列
    MoveVector3 _vector3; // 存储当前移动向量信息
};
