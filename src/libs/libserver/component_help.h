#pragma once

#include "entity_system.h" // 引入 EntitySystem 类
#include "yaml.h"          // 引入 Yaml 类，用于 YAML 文件处理
#include "res_path.h"      // 引入 ResPath 类，用于资源路径管理
#include "trace_component.h" // 引入 TraceComponent 类，用于追踪和调试信息

// ComponentHelp 类提供一些静态方法，方便访问全局组件和资源
class ComponentHelp
{
public:
    // 获取全局的 EntitySystem 实例
    static EntitySystem* GetGlobalEntitySystem();

    // 获取 Yaml 实例，用于配置和序列化
    static Yaml* GetYaml();

    // 获取资源路径管理实例
    static ResPath* GetResPath();

    // 获取追踪组件实例，用于调试信息
    static TraceComponent* GetTraceComponent();

#if ENGINE_PLATFORM != PLATFORM_WIN32    
    // 捕获错误并打印信息（适用于非 Windows 平台）
    static void CatchError(bool bResult); // 输出当前状态
#endif

};
