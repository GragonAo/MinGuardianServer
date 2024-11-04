#include "move_component.h"                       // 包含移动组件的定义
#include "libplayer/player.h"                     // 包含玩家的定义
#include "libplayer/player_component_last_map.h"  // 包含玩家最后地图组件的定义

#include <cmath>                                  // 包含数学函数库，例如 sqrt

// 更新移动向量，计算目标方向和比例
void MoveVector3::Update(Vector3 target, Vector3 position)
{
    Target = target; // 设置目标点

    // 计算目标点与当前位置的距离
    const auto xdis = target.X - position.X;  // 目标点与当前位置在X轴的距离
    const auto zdis = target.Z - position.Z;  // 目标点与当前位置在Z轴的距离
    const auto distance = sqrt(xdis * xdis + zdis * zdis); // 计算两点之间的欧几里得距离

    // 根据距离计算在X和Z方向上的比例
    ScaleX = xdis / distance; // 计算X轴方向的移动比例
    ScaleZ = zdis / distance; // 计算Z轴方向的移动比例
}

// 初始化移动组件，清空目标队列
void MoveComponent::Awake()
{
    while (!_targets.empty()) // 当目标队列不为空
        _targets.pop(); // 清空队列
}

// 将移动组件返回池中，清空目标队列
void MoveComponent::BackToPool()
{
    while (!_targets.empty()) // 当目标队列不为空
        _targets.pop(); // 清空队列
}

// 更新移动组件，接收目标队列和当前位置
void MoveComponent::Update(std::queue<Vector3> targets, Vector3 curPosition)
{
    if (targets.empty()) // 如果目标队列为空
        return; // 直接返回

    // 清空当前目标队列
    while (!_targets.empty())
    {
        _targets.pop();
    }

    // 交换目标队列
    std::swap(targets, _targets); // 将传入的目标队列与当前队列交换

    auto v3 = _targets.front(); // 获取当前队列的第一个目标
    _vector3.Update(v3, curPosition); // 更新移动向量
    _targets.pop(); // 移除已处理的目标
}

// 更新位置，根据经过的时间和速度移动玩家
bool MoveComponent::Update(const float timeElapsed, PlayerComponentLastMap* pLastMap, const float speed)
{
    auto curPosition = pLastMap->GetCur()->Position; // 获取当前位置

    // 计算根据经过的时间和速度可以移动的距离
    const auto moveDis = timeElapsed * 0.001 * speed; // moveDis 是单位时间内的移动距离

    bool isStop = false; // 标记是否停止移动
    if (curPosition.GetDistance(_vector3.Target) < moveDis) // 如果当前位置与目标点的距离小于可移动距离
    {
        // 已到达目标点
        curPosition = _vector3.Target; // 将当前位置设为目标点

        // 处理下一个目标点
        if (!_targets.empty()) // 如果还有目标
        {
            const auto v3 = _targets.front(); // 获取下一个目标
            _vector3.Update(v3, curPosition); // 更新移动向量
            _targets.pop(); // 移除已处理的目标
        }
        else
        {
            isStop = true; // 如果没有更多目标，标记为停止
        }
    } 
    else
    {
        // 根据比例移动
        const auto xDis = moveDis * _vector3.ScaleX; // 在X轴上的移动量
        const auto zDis = moveDis * _vector3.ScaleZ; // 在Z轴上的移动量

        curPosition.X += xDis; // 更新当前位置的X坐标
        curPosition.Z += zDis; // 更新当前位置的Z坐标
    }

    // 更新最后地图组件中的当前位置
    pLastMap->GetCur()->Position = curPosition; 
    LOG_DEBUG("cur position. " << curPosition); // 打印当前位置信息

    return isStop; // 返回是否停止移动的状态
}
