#pragma once

#include "system.h"
#include "component_collections.h"

// 更新系统类
class UpdateSystem : virtual public ISystem<UpdateSystem>
{
public:
    // 更新方法，接受一个实体系统的指针
    void Update(EntitySystem* pEntities) override;

private:
    // 组件集合指针，初始为空
    ComponentCollections* _pCollections{ nullptr };
};
