#pragma once
#include <algorithm>
#include <list>
#include <map>
#include <queue>

#include "log4_help.h" // 日志帮助文件，用于记录日志
#include "disposable.h" // IDisposable 接口文件

// 模板类 CacheRefresh 用于管理缓存的刷新操作，继承自 IDisposable
template<class T>
class CacheRefresh : public IDisposable
{
public:
    // 获取当前缓存中的对象，返回一个指向对象的 map
    std::map<uint64, T*>* GetReaderCache();

    // 添加对象到新增缓存中
    void AddObj(T* pObj);

    // 从缓存中移除对象（通过对象的唯一标识符 sn）
    void RemoveObj(uint64 sn);

    // 获取缓存中的对象总数（包括已添加的对象）
    int Count();

    // 交换操作：处理已添加和移除的对象，移除对象是否回收到回收列表取决于参数
    void Swap(std::queue<T*>* pRecycleList);

    // 检查是否有对象可以进行交换操作
    bool CanSwap();

    // 将所有对象返回到对象池中，用于资源回收
    void BackToPool();

    // 重写 IDisposable 接口的 Dispose 方法，销毁缓存中的对象
    void Dispose() override;

protected:
    std::map<uint64, T*> _objs;    // 缓存中当前存储的对象
    std::map<uint64, T*> _adds;    // 新增的对象集合
    std::list<uint64> _removes;    // 待移除的对象唯一标识符集合
};

// 获取缓存中存储的对象
template <class T>
inline std::map<uint64, T*>* CacheRefresh<T>::GetReaderCache()
{
    return &_objs;
}

// 添加对象到新增缓存中，key 为对象的唯一标识符 sn
template<class T>
inline void CacheRefresh<T>::AddObj(T* pObj)
{
    _adds.emplace(std::make_pair(pObj->GetSN(), pObj));
}

// 将对象标记为待移除，通过传入对象的唯一标识符 sn
template<class T>
inline void CacheRefresh<T>::RemoveObj(uint64 sn)
{
    _removes.emplace_back(sn);
}

// 获取当前缓存中对象的总数，包括新增的对象
template<class T>
inline int CacheRefresh<T>::Count()
{
    return _objs.size() + _adds.size();
}

// 交换操作：将新增对象加入到主缓存，并处理待移除对象
template <class T>
void CacheRefresh<T>::Swap(std::queue<T*>* pRecycleList)
{
    // 如果有新增对象，将其添加到主缓存中
    if (!_adds.empty())
    {
        _objs.insert(_adds.begin(), _adds.end());
        _adds.clear(); // 清空新增缓存
    }

    // 如果有待移除对象，处理移除逻辑
    if (!_removes.empty()) {
        for (auto one : _removes)
        {
            auto iter = _objs.find(one);
            if (iter == _objs.end())
            {
                LOG_WARN("CacheRefresh Swap error. not find obj to remove. type:" << typeid(T).name() << " sn:" << one);
            }
            else
            {
                // 如果回收列表不为空，将对象放入回收队列
                if (pRecycleList != nullptr)
                {
                    iter->second->SetSN(0); // 重置对象的唯一标识符
                    pRecycleList->emplace(iter->second);
                } 
                else
                {
                    delete iter->second; // 如果没有回收列表，直接删除对象
                }

                _objs.erase(iter); // 从主缓存中移除对象
            }
        }
        _removes.clear(); // 清空待移除对象列表
    }
}

// 判断是否有对象可以进行交换操作
template <class T>
inline bool CacheRefresh<T>::CanSwap()
{
    return !_adds.empty() || !_removes.empty();
}

// 将所有对象返回到对象池中，用于对象的资源回收
template<class T>
inline void CacheRefresh<T>::BackToPool()
{
    // 遍历新增缓存，将对象返回到对象池
    for (auto iter = _adds.begin(); iter != _adds.end(); ++iter)
    {
        iter->second->ComponentBackToPool();
    }
    _adds.clear(); // 清空新增缓存

    // 遍历主缓存，将对象返回到对象池
    for (auto iter = _objs.begin(); iter != _objs.end(); ++iter)
    {
        iter->second->ComponentBackToPool();
    }
    _objs.clear(); // 清空主缓存

    _removes.clear(); // 清空待移除列表
}

// 释放对象资源并销毁缓存
template <class T>
void CacheRefresh<T>::Dispose()
{
    // 释放新增缓存中的对象
    for (auto iter = _adds.begin(); iter != _adds.end(); ++iter)
    {
        auto pObj = iter->second;
        pObj->Dispose(); // 调用对象的 Dispose 方法
        delete pObj; // 删除对象
    }
    _adds.clear(); // 清空新增缓存

    // 释放主缓存中的对象
    for (auto iter = _objs.begin(); iter != _objs.end(); ++iter)
    {
        auto pObj = iter->second;
        pObj->Dispose(); // 调用对象的 Dispose 方法
        delete pObj; // 删除对象
    }
    _objs.clear(); // 清空主缓存
    _removes.clear(); // 清空待移除列表
}
