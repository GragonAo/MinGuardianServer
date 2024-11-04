#include "util_string.h"

// 不区分大小写的字符串比较
// 参数:
//   c1: 第一个字符串指针。
//   c2: 第二个字符串指针。
// 返回值: 
//   如果 c1 小于 c2，返回负值；
//   如果 c1 等于 c2，返回 0；
//   如果 c1 大于 c2，返回正值。
int strutil::stricmp(char const* c1, char const* c2)
{
    // 使用自定义的 engine_stricmp 函数进行字符串比较
    return engine_stricmp(c1, c2);
}

// 格式化字符串
// 参数:
//   _format: 使用 printf 风格的格式化字符串。
//   ...: 可变参数，按照 _format 指定的格式传入。
// 返回值:
//   格式化后的字符串。
std::string strutil::format(const char* _format, ...)
{
    std::string str; // 用于存储格式化后的字符串
    va_list args1; // 定义可变参数列表
    va_start(args1, _format); // 初始化 args1，以便从 _format 开始获取参数

    va_list args2; // 定义第二个可变参数列表
    va_copy(args2, args1); // 复制 args1 到 args2，以便后续使用

    // 计算格式化后所需的字符数量（包括终止符）
    const size_t num_of_chars = std::vsnprintf(nullptr, 0, _format, args1);
    
    // 如果所需的字符数量大于当前字符串的容量，调整字符串的大小
    if (num_of_chars > str.capacity())
    {
        str.resize(num_of_chars + 1); // +1 以容纳终止符
    }

    // 使用 vsnprintf 将格式化的字符串写入 str 中
    std::vsnprintf(const_cast<char*>(str.data()), str.capacity(), _format, args2);

    // 清理可变参数列表
    va_end(args1);
    va_end(args2);

    return str; // 返回格式化后的字符串
}
