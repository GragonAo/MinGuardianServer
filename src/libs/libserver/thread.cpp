#include "thread.h"
#include "global.h"
#include "entity_system.h"
#include "efficiency_thread_component.h"

#include <iterator>

// 线程类的实现
Thread::Thread(ThreadType threadType)
{
    _state = ThreadState::Init; // 初始化线程状态
    _threadType = threadType;    // 设置线程类型
}

Thread::~Thread()
{
    // 析构函数，执行清理操作
    // std::cout << "close thread [3/3] delete. " << std::endl;
}

void Thread::Start()
{
    // 启动线程
    _thread = std::thread([this]()
        {
            InitComponent(_threadType); // 初始化组件
            _state = ThreadState::Run;   // 更新状态为运行中
            const auto pGlobal = Global::GetInstance(); // 获取全局实例

#if LOG_EFFICIENCY_COMPONENT_OPEN
            // 添加效率线程组件
            auto pObj = this->GetEntitySystem()->AddComponent<EfficiencyThreadComponent>(_threadType, _thread.get_id());
            timeutil::Time start = 0; // 初始化开始时间
#endif

            // 主循环，直到全局停止标志为真
            while (!pGlobal->IsStop)
            {
#if LOG_EFFICIENCY_COMPONENT_OPEN
                start = pGlobal->TimeTick; // 记录当前时间
#endif

                Update(); // 更新线程状态

#if LOG_EFFICIENCY_COMPONENT_OPEN
                // 更新组件的运行时间
                pObj->UpdateTime(pGlobal->TimeTick - start);
#endif

                std::this_thread::sleep_for(std::chrono::milliseconds(1)); // 暂停1毫秒
            }

            // std::cout << "close thread [1/3] stop. thread id:" << _thread.get_id() << std::endl;

            Dispose(); // 释放资源
            log4cplus::threadCleanup(); // 清理线程
            _state = ThreadState::Stop; // 更新状态为停止
        });
}

bool Thread::IsStop() const
{
    return _state == ThreadState::Stop; // 判断线程是否已停止
}

bool Thread::IsDestroy() const
{
    return _state == ThreadState::Destroy; // 判断线程是否已销毁
}

void Thread::DestroyThread()
{
    // 销毁线程
    if (_state == ThreadState::Destroy)
        return; // 如果已经销毁，直接返回

    if (_thread.joinable())
    {
        // std::cout << "close thread [2/3] join. thread id:" << _thread.get_id() << std::endl;
        _thread.join(); // 等待线程结束
        _state = ThreadState::Destroy; // 更新状态为销毁
    }
}

void Thread::Dispose()
{
    // 释放资源
    if (_state == ThreadState::Destroy)
        return; // 如果已经销毁，直接返回

    SystemManager::Dispose(); // 调用基类的 Dispose
}
