#include "component_collections.h"
#include <utility>
#include "log4_help.h"

ComponentCollections::ComponentCollections(std::string componentName)
{
    _componentName = componentName;
}

ComponentCollections::~ComponentCollections()
{
    if (!_objs.empty() || !_addObjs.empty())
    {
        std::cout << " class:" << _componentName.c_str() << " still reachable. " << std::endl;
    }

    _removeObjs.clear();
}

void ComponentCollections::Add(IComponent* pObj)
{
    // 检查是否已有相同SN的组件，避免重复添加
    if (_objs.find(pObj->GetSN()) != _objs.end() || _addObjs.find(pObj->GetSN()) != _addObjs.end())
    {
        LOG_ERROR("add component error. Repeat SN. type:" << pObj->GetTypeName() << " sn:" << pObj->GetSN() << " type:" << this->GetClassType().c_str());
        return;
    }

    // 添加新组件到_addObjs暂存区
    _addObjs[pObj->GetSN()] = pObj;
}

IComponent* ComponentCollections::Get(const uint64 sn)
{
    if (sn == 0)
    {
        // 如果sn为0，返回_objs或_addObjs中第一个组件
        if (!_objs.empty())
            return _objs.begin()->second;

        if (!_addObjs.empty())
            return _addObjs.begin()->second;
    }
    else
    {
        // 根据sn获取组件
        const auto iter1 = _objs.find(sn);
        if (iter1 != _objs.end())
            return iter1->second;

        const auto iter2 = _addObjs.find(sn);
        if (iter2 != _addObjs.end())
            return iter2->second;
    }
    return nullptr;
}

void ComponentCollections::Remove(uint64 sn)
{
    // 将sn添加到_removeObjs列表，待后续批量删除
    _removeObjs.emplace_back(sn);
}

std::map<uint64, IComponent*>* ComponentCollections::GetAll()
{
    return &_objs;
}

void ComponentCollections::Swap()
{
    // 将_addObjs中的组件添加到_objs
    if (!_addObjs.empty())
    {
        for (auto pair : _addObjs)
        {
            _objs.insert(std::make_pair(pair.first, pair.second));
        }
        _addObjs.clear();
    }

    // 根据_removeObjs中记录的sn移除组件
    if (!_removeObjs.empty())
    {
        for (auto sn : _removeObjs)
        {
            const auto iter = _objs.find(sn);
            if (iter != _objs.end())
            {
#if LOG_SYSOBJ_OPEN
                if (iter->second->GetTypeHashCode() != typeid(Packet).hash_code())
                    LOG_SYSOBJ("*[sys] dispose obj. obj sn:" << iter->second->GetSN() << " type:" << iter->second->GetTypeName() << " thead id:" << std::this_thread::get_id());
#endif
                // 释放组件并从_objs中移除
                iter->second->ComponentBackToPool();
                _objs.erase(iter);
                continue;
            }

            // 无法找到要移除的组件，记录警告日志
            LOG_WARN("remove obj failed. sn:" << sn << " type:" << this->GetClassType().c_str());
        }

        _removeObjs.clear();
    }
}

// 释放所有对象，并清空集合
void ComponentCollections::Dispose()
{
    // 释放_objs中的组件
    for (const auto pair : _objs)
    {
        auto pComponent = pair.second;
        if (pComponent->GetSN() == 0)
            continue;

        pComponent->ComponentBackToPool();
    }

    _objs.clear();

    // 释放_addObjs中的组件
    for (const auto pair : _addObjs)
    {
        auto pComponent = pair.second;
        if (pComponent->GetSN() == 0)
            continue;

        pComponent->ComponentBackToPool();
    }

    _addObjs.clear();
    _removeObjs.clear();
}

std::string ComponentCollections::GetClassType() const
{
    return _componentName;
}
