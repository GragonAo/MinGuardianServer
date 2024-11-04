#include "component_help.h" // 包含 ComponentHelp 的声明
#include "thread_mgr.h"     // 包含线程管理器的头文件

#if ENGINE_PLATFORM != PLATFORM_WIN32
#include <execinfo.h>      // 包含用于堆栈跟踪的头文件（仅在非 Windows 平台上）
#endif

// 获取全局的 EntitySystem 实例
EntitySystem* ComponentHelp::GetGlobalEntitySystem()
{
    return ThreadMgr::GetInstance()->GetEntitySystem(); // 通过线程管理器获取 EntitySystem
}

// 获取 Yaml 实例
Yaml* ComponentHelp::GetYaml()
{
    return ThreadMgr::GetInstance()->GetEntitySystem()->GetComponent<Yaml>(); // 从 EntitySystem 中获取 Yaml 组件
}

// 获取资源路径管理实例
ResPath* ComponentHelp::GetResPath()
{
    return ThreadMgr::GetInstance()->GetEntitySystem()->GetComponent<ResPath>(); // 从 EntitySystem 中获取 ResPath 组件
}

// 获取追踪组件实例
TraceComponent* ComponentHelp::GetTraceComponent()
{
    return ThreadMgr::GetInstance()->GetEntitySystem()->GetComponent<TraceComponent>(); // 从 EntitySystem 中获取 TraceComponent 组件
}

#if ENGINE_PLATFORM != PLATFORM_WIN32
// 捕获错误并打印堆栈信息（适用于非 Windows 平台）
void ComponentHelp::CatchError(bool bResult)
{
    if (bResult) // 如果 bResult 为 true，直接返回
        return;

    void* array[10]; // 存储堆栈地址
    size_t size; // 实际获得的堆栈帧数
    char** strings; // 用于保存堆栈符号的字符串数组
    size_t i;

    size = backtrace(array, 10); // 获取当前的堆栈信息
    strings = backtrace_symbols(array, size); // 将堆栈地址转换为符号

    std::stringstream ss; // 用于构建输出信息的字符串流
    ss << "Obtained " << size << " stack frames.\n"; // 输出堆栈帧数信息

    // 遍历所有堆栈帧并将其添加到字符串流
    for (i = 0; i < size; i++)
        ss << strings[i] << "\n";
        
    LOG_ERROR(ss.str().c_str()); // 将堆栈信息记录为错误日志
    free(strings); // 释放字符串数组
}
#endif
