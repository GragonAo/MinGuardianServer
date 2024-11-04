#include "resource_manager.h"        // 引入资源管理器的头文件
#include "libserver/log4.h"          // 引入日志库
#include "libserver/component_help.h" // 引入组件帮助库

// 初始化资源管理器
void ResourceManager::Awake()
{
    // 获取资源路径
    const auto pResPath = ComponentHelp::GetResPath();
    
    // 创建资源世界管理器的实例
    Worlds = new ResourceWorldMgr();
    
    // 初始化资源世界管理器
    if (!Worlds->Initialize("world", pResPath))
    {
        LOG_ERROR("world txt Initialize. failed."); // 如果初始化失败，记录错误日志
    }

    LOG_COLOR(LogColorYellowEx, "all resource loaded."); // 记录成功加载资源的日志
}

// 返回对象池
void ResourceManager::BackToPool()
{
    // 这里可以实现返回对象池的逻辑
}
