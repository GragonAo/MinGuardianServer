#pragma once

#include <thread>
#include <list>

#include "sn_object.h"
#include "system_manager.h"
#include "thread_type.h"

// 线程状态枚举
enum class ThreadState
{
    Init,    // 初始化
    Run,     // 运行中
    Stop,    // 停止
    Destroy, // 销毁
};

// 线程类，继承自 SystemManager 和 SnObject
class Thread : public SystemManager, public SnObject
{
public:
    // 构造函数，接受线程类型
    Thread(ThreadType threadType);
    
    // 析构函数
    ~Thread();

    // 启动线程
    void Start();
    
    // 销毁线程
    void DestroyThread();
    
    // 释放资源，重写自 IDisposable
    void Dispose() override;

    // 判断线程是否已停止
    bool IsStop() const;    
    
    // 判断线程是否已销毁
    bool IsDestroy() const;

    // 获取线程类型
    ThreadType GetThreadType() const { return _threadType; }

private:
    ThreadType _threadType;  // 线程类型

    ThreadState _state;      // 线程状态
    std::thread _thread;     // 线程对象
};
