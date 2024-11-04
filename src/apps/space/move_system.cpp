#include "move_system.h"                         // 包含 MoveSystem 类的头文件
#include "move_component.h"                      // 包含 MoveComponent 类的头文件
#include "libserver/entity_system.h"            // 包含 EntitySystem 类的头文件
#include "libplayer/player.h"                    // 包含 Player 类的头文件
#include "libplayer/player_component_last_map.h" // 包含 PlayerComponentLastMap 类的头文件

// MoveSystem 类的构造函数
MoveSystem::MoveSystem()
{
    _lastTime = Global::GetInstance()->TimeTick; // 初始化上一次更新时间为当前时间
}

// 更新系统的方法
void MoveSystem::Update(EntitySystem* pEntities)
{
    // 每 0.5 秒更新一次
    const auto curTime = Global::GetInstance()->TimeTick; // 获取当前时间
    const auto timeElapsed = curTime - _lastTime; // 计算自上次更新以来的时间
    if (timeElapsed < 500) // 如果时间小于 500 毫秒，直接返回
        return;

    // 如果组件集合未初始化，则进行初始化
    if (_pCollections == nullptr)
    {
        _pCollections = pEntities->GetComponentCollections<MoveComponent>(); // 获取 MoveComponent 的组件集合
        if (_pCollections == nullptr) // 如果获取失败，则返回
            return;
    }

    _lastTime = curTime; // 更新上一次更新时间

    const auto plists = _pCollections->GetAll(); // 获取所有的移动组件
    for (auto iter = plists->begin(); iter != plists->end(); ++iter) // 遍历每个移动组件
    {
        auto pMoveComponent = dynamic_cast<MoveComponent*>(iter->second); // 将组件转换为 MoveComponent 类型
        auto pPlayer = pMoveComponent->GetParent<Player>(); // 获取与该移动组件相关联的 Player

        // 更新移动组件并检查是否需要移除
        if (pMoveComponent->Update(timeElapsed, pPlayer->GetComponent<PlayerComponentLastMap>(), 2))
        {
            pPlayer->RemoveComponent<MoveComponent>(); // 如果移动组件更新后返回 true，则从 Player 中移除该组件
        }
    }
}
