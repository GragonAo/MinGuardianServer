#pragma once
#include <queue>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "sn_object.h"
#include "object_pool_interface.h"
#include "cache_refresh.h"
#include "log4_help.h"
#include "system_manager.h"
#include "global.h"

// 动态对象池模板类，继承自 IDynamicObjectPool
template <typename T>
class DynamicObjectPool : public IDynamicObjectPool
{
public:
    void Dispose() override;  // 释放资源

    // 从池中分配对象
    template<typename ...Targs>
    T* MallocObject(SystemManager* pSys, IEntity* pParent, uint64 sn, Targs... args);

    virtual void Update() override;  // 更新对象池
    virtual void FreeObject(IComponent* pObj) override;  // 释放对象

    virtual void Show() override;  // 显示对象池状态

protected:
    std::queue<T*> _free;  // 可用对象队列
    CacheRefresh<T> _objInUse;  // 使用中的对象缓存

#if _DEBUG
    int _totalCall{ 0 };  // 调用计数
#endif
};

// 释放对象池中的资源
template <typename T>
void DynamicObjectPool<T>::Dispose()
{
    if (_objInUse.Count() > 0)
    {
        std::cout << "delete pool. " << typeid(T).name() << " count:" << _objInUse.Count() << std::endl;
    }

    // 删除所有空闲对象
    while (!_free.empty())
    {
        auto obj = _free.front();
        delete obj;
        _free.pop();
    }
}

// 从池中分配对象的实现
template <typename T>
template <typename ... Targs>
T* DynamicObjectPool<T>::MallocObject(SystemManager* pSys, IEntity* pParent, uint64 sn, Targs... args)
{
    // 如果没有可用对象，则创建新对象
    if (_free.empty())
    {
        if (T::IsSingle())
        {
            T* pObj = new T();
            pObj->SetSN(0);
            pObj->SetPool(this);
            _free.push(pObj);
        }
        else
        {
            // 创建50个新对象
            for (int index = 0; index < 50; index++)
            {
                T* pObj = new T();
                pObj->SetSN(0);
                pObj->SetPool(this);
                _free.push(pObj);
            }
        }
    }

#if _DEBUG
    _totalCall++;  // 增加调用计数
#endif

    auto pObj = _free.front();  // 获取空闲对象
    _free.pop();

    if (pObj->GetSN() != 0)
    {
        LOG_ERROR("failed to create type:" << typeid(T).name() << " sn != 0. sn:" << pObj->GetSN());
    }

    // 如果 SN 为 0，则生成新的 SN
    if (sn == 0)
        sn = Global::GetInstance()->GenerateSN();

    // 初始化对象
    pObj->SetSN(sn);
    pObj->SetPool(this);
    pObj->SetParent(pParent);
    pObj->SetSystemManager(pSys);
    pObj->Awake(std::forward<Targs>(args)...);

#if LOG_SYSOBJ_OPEN
    LOG_SYSOBJ("*[pool] awake obj. obj sn:" << pObj->GetSN() << " type:" << pObj->GetTypeName() << " thread id:" << std::this_thread::get_id());
#endif

    _objInUse.AddObj(pObj);  // 将对象添加到使用中
    return pObj;  // 返回分配的对象
}

// 更新对象池的实现
template <typename T>
void DynamicObjectPool<T>::Update()
{
    if (_objInUse.CanSwap())
    {
        _objInUse.Swap(&_free);  // 交换使用中的对象和空闲对象
    }
}

// 释放对象的实现
template<typename T>
inline void DynamicObjectPool<T>::FreeObject(IComponent* pObj)
{
    if (pObj->GetSN() == 0)
    {
        LOG_ERROR("free obj sn == 0. type:" << typeid(T).name());
        return;
    }

#if LOG_SYSOBJ_OPEN
    LOG_SYSOBJ("*[pool] free obj. obj sn:" << pObj->GetSN() << " type:" << pObj->GetTypeName() << " thread id:" << std::this_thread::get_id());
#endif

    _objInUse.RemoveObj(pObj->GetSN());  // 从使用中移除对象
}

// 显示对象池状态的实现
template <typename T>
void DynamicObjectPool<T>::Show()
{
    std::stringstream log;
    log << " total:" << std::setw(5) << std::setfill(' ') << _free.size() + _objInUse.Count()

#if _DEBUG
        << "    call:" << std::setw(5) << std::setfill(' ') << _totalCall
#endif

        << "    free:" << std::setw(5) << std::setfill(' ') << _free.size()
        << "    use:" << std::setw(5) << std::setfill(' ') << _objInUse.Count()
        << "    " << typeid(T).name();

    LOG_DEBUG(log.str().c_str());  // 打印对象池状态
}
