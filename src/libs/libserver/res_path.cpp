#include "res_path.h" // 包含 ResPath 的头文件
#include <ostream> // 处理输出流
#include "common.h" // 包含常用的头文件
#include "util_string.h" // 包含字符串处理的工具

#if ENGINE_PLATFORM != PLATFORM_WIN32	
#include <unistd.h> // POSIX 标准的头文件
#endif

// 初始化方法，设置资源路径
void ResPath::Awake()
{
    // 从环境变量获取资源路径
#if ENGINE_PLATFORM != PLATFORM_WIN32		
    char* envValue = getenv("GENGINE_RES_PATH");
    _resPath = envValue == nullptr ? "" : envValue;
#else
    char* envValue = nullptr;
    size_t len;
    // Windows 平台获取环境变量
    if (_dupenv_s(&envValue, &len, "GENGINE_RES_PATH") == 0 && envValue != nullptr)
    {
        _resPath = envValue;
        free(envValue); // 释放分配的内存
    }
#endif

    // 如果环境变量为空，获取默认路径
    if (_resPath.empty())
    {
#if ENGINE_PLATFORM != PLATFORM_WIN32		
        char name[PATH_MAX];
        // 读取当前执行文件的路径
        int rslt = ::readlink("/proc/self/exe", name, PATH_MAX);
        if (rslt < 0 || rslt >= PATH_MAX)
        {
            std::cout << "ResPath::Initialize. res path failed." << std::endl;
            return;
        }

        name[rslt] = 0; // 添加字符串结束符
        _resPath = name;

        // 移除文件名和扩展名，保留目录路径
        for (int i = 0; i < 2; i++)
        {
            const auto index = _resPath.find_last_of("/");
            _resPath = _resPath.substr(0, index);
        }
#else
        // Windows 平台获取模块文件名
        char name[PATH_MAX];
        ZeroMemory(name, PATH_MAX); // 清空内存
        const DWORD rslt = GetModuleFileNameA(nullptr, name, MAX_PATH);
        if (rslt <= 0 || rslt >= PATH_MAX)
        {
            std::cout << "ResPath::Initialize. res path failed." << std::endl;
            return;
        }

        _resPath = name;

        // 替换路径分隔符
        strutil::replace(_resPath, "\\", "/");
        strutil::replace(_resPath, "//", "/");

        // 移除文件名和扩展名，保留目录路径
        for (int i = 0; i < 3; i++)
        {
            const auto index = _resPath.find_last_of("/");
            _resPath = _resPath.substr(0, index);
        }
#endif
    }

    // 添加默认资源目录
    _resPath += "/res";
    std::cout << "GENGINE_RES_PATH=" << _resPath << std::endl; // 输出资源路径
}

// 返回对象池的方法（当前未实现）
void ResPath::BackToPool()
{
    // 当前方法未实现，后续可以根据需求实现对象池的返回逻辑
}

// 根据资源名称查找资源路径
std::string ResPath::FindResPath(const std::string& res)
{
    return FindResPath(res.c_str()); // 调用重载的函数
}

// 根据 C 风格字符串查找资源路径
std::string ResPath::FindResPath(const char* res)
{
    auto fpath = _resPath + res; // 拼接路径

    // 替换路径分隔符
    strutil::replace(fpath, "\\", "/");
    strutil::replace(fpath, "//", "/");

    return fpath; // 返回拼接后的完整路径
}
