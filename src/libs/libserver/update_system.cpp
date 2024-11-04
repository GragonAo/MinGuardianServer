#include "update_system.h"
#include "update_component.h"
#include "entity_system.h"

// 更新系统
void UpdateSystem::Update(EntitySystem* pEntities)
{
    // 如果组件集合为空，获取更新组件集合
    if (_pCollections == nullptr)
    {
        _pCollections = pEntities->GetComponentCollections<UpdateComponent>();
        // 如果仍然为空，则返回
        if (_pCollections == nullptr)
            return;
    }

    // 交换当前组件集合
    _pCollections->Swap();
    // 获取所有更新组件的列表
    const auto plists = _pCollections->GetAll();
    // 遍历所有更新组件
    for (auto iter = plists->begin(); iter != plists->end(); ++iter)
    {
        // 动态_cast获取更新组件指针
        const auto pUpdateComponent = dynamic_cast<UpdateComponent*>(iter->second);
        // 调用更新方法
        pUpdateComponent->Update();
    }
}
