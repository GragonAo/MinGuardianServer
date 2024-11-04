#pragma once
#include <list>    // 引入 STL 标准库中的 list 容器
#include <mutex>   // 引入互斥锁类（目前未使用，但可以为将来扩展提供线程安全支持）

template<class T>
class CacheSwap
{
public:
    // 构造函数，初始化读写缓存
    CacheSwap()
    {
        _writerCache = &_caches1; // 将 _writerCache 指向第一个缓存
        _readerCache = &_caches2; // 将 _readerCache 指向第二个缓存
    }

    // 获取写缓存的指针
    std::list<T*>* GetWriterCache();
    
    // 获取读缓存的指针
    std::list<T*>* GetReaderCache();
    
    // 交换读写缓存
    void Swap();
    
    // 判断当前写缓存是否可以交换（是否有数据）
    bool CanSwap();
    
    // 将所有缓存中的对象返回池中（清理操作）
    void BackToPool();

private:
    std::list<T*> _caches1;      // 缓存1，作为写缓存
    std::list<T*> _caches2;      // 缓存2，作为读缓存

    std::list<T*>* _readerCache; // 当前读缓存的指针
    std::list<T*>* _writerCache; // 当前写缓存的指针
};

// 获取写缓存的指针
template <class T>
inline std::list<T*>* CacheSwap<T>::GetWriterCache()
{
    return _writerCache;
}

// 获取读缓存的指针
template <class T>
inline std::list<T*>* CacheSwap<T>::GetReaderCache()
{
    return _readerCache;
}

// 交换读写缓存的指针
template <class T>
inline void CacheSwap<T>::Swap()
{
    auto tmp = _readerCache; // 暂存当前读缓存
    _readerCache = _writerCache; // 将写缓存指针赋值给读缓存
    _writerCache = tmp; // 将之前的读缓存指针赋值给写缓存
}

// 判断当前写缓存是否可以交换
template <class T>
inline bool CacheSwap<T>::CanSwap()
{
    return _writerCache->size() > 0; // 如果写缓存中有数据，返回 true
}

// 将所有缓存中的对象返回到池中，执行清理操作
template<class T>
inline void CacheSwap<T>::BackToPool()
{
    // 遍历缓存1并返回每个对象到池中
    for (auto iter = _caches1.begin(); iter != _caches1.end(); ++iter)
    {
        (*iter)->BackToPool(); // 调用对象的 BackToPool 方法
    }
    _caches1.clear(); // 清空缓存1

    // 遍历缓存2并返回每个对象到池中
    for (auto iter = _caches2.begin(); iter != _caches2.end(); ++iter)
    {
        (*iter)->BackToPool(); // 调用对象的 BackToPool 方法
    }
    _caches2.clear(); // 清空缓存2
}
