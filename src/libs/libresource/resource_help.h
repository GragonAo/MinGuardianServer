#pragma once
#include "resource_manager.h" // 引入资源管理器的头文件

class ResourceHelp
{
public:
    // 静态方法，用于获取资源管理器的实例
    static ResourceManager* GetResourceManager();
};
