#pragma once // 确保头文件只被包含一次

#include "component.h" // 包含 Component 基类
#include "system.h"    // 包含系统类

// ResPath 类用于管理资源路径
class ResPath : public Component<ResPath>, public IAwakeSystem<> // 继承自 Component 和 IAwakeSystem
{
public:
    // 组件初始化方法
    void Awake();
    
    // 组件返回对象池的方法
    void BackToPool();

    // 根据资源名称查找资源路径
    std::string FindResPath(const std::string& res);
    
    // 根据资源名称查找资源路径（C风格字符串）
    std::string FindResPath(const char* res);

private:
    std::string _resPath{ "" }; // 存储资源路径的成员变量，默认为空字符串
};
