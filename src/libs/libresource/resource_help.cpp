#include "resource_help.h"
#include "libserver/thread_mgr.h" // 引入线程管理器的头文件

// 获取资源管理器的实例
ResourceManager* ResourceHelp::GetResourceManager()
{
    // 从线程管理器获取实体系统，并获取资源管理器组件
    return ThreadMgr::GetInstance()->GetEntitySystem()->GetComponent<ResourceManager>();
}
