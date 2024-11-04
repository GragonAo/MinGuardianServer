#pragma once
#include "component.h"
#include "disposable.h"

#include <list>
#include <map>
#include <string>

// ComponentCollections 是用于管理组件对象的集合类
class ComponentCollections : public IDisposable
{
public:
    // 构造函数，传入组件的类名
    ComponentCollections(std::string componentName);
    ~ComponentCollections();

    // 添加一个组件对象到集合
    void Add(IComponent* pObj);

    // 通过唯一标识符（sn）移除组件
    void Remove(uint64 sn);

    // 获取组件对象，默认使用 sn=0 获取第一个
    IComponent* Get(uint64 sn = 0);

    // 获取所有组件对象的映射
    std::map<uint64, IComponent*>* GetAll();

    // 交换添加与移除的组件
    void Swap();

    // 释放资源，清空集合
    void Dispose() override;

    // 获取组件类型名称
    std::string GetClassType() const;

private:
    // 当前有效的组件对象集合，使用唯一标识符（sn）作为键
    std::map<uint64, IComponent*> _objs;

    // 暂存添加的组件对象，等到 Swap 时正式加入 _objs
    std::map<uint64, IComponent*> _addObjs;

    // 记录待移除的组件对象的唯一标识符
    std::list<uint64> _removeObjs;

    // 组件的类名
    std::string _componentName{ "" };
};
