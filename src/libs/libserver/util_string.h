#pragma once

#include <string>
#include <vector>
#include <cstdarg>  // 用于处理可变参数的函数

#include "common.h" // 假设该头文件中包含了一些常用的定义和类型

namespace strutil
{
    // 不区分大小写的字符串比较
    // 返回值：如果 c1 小于 c2，返回负值；如果 c1 等于 c2，返回 0；如果 c1 大于 c2，返回正值。
    int stricmp(char const* c1, char const* c2);

    // 格式化字符串
    // 参数:
    //   _format: 格式化字符串，使用 printf 风格的格式。
    //   ...: 可变参数，按照 _format 指定的格式传入。
    // 返回值：格式化后的字符串。
    std::string format(const char* _format, ...);

    // 去除字符串首尾空白字符
    // 参数:
    //   s: 需要处理的字符串。
    // 返回值：去除首尾空白后的字符串。
    inline std::string trim(const std::string& s) {
        // 定义正向和反向迭代器
        std::string::const_iterator it = s.begin();
        // 移动正向迭代器，直到找到第一个非空白字符
        while (it != s.end() && isspace(*it))
            ++it;

        // 反向迭代器，直到找到第一个非空白字符
        std::string::const_reverse_iterator rit = s.rbegin();
        while (rit.base() != it && isspace(*rit))
            ++rit;

        // 返回去除空白后的子字符串
        return std::string(it, rit.base());
    }

    // 字符串替换
    // 参数:
    //   str: 需要进行替换操作的字符串。
    //   pattern: 需要被替换的子串。
    //   newpat: 替换成的新子串。
    // 返回值：替换操作的次数。
    inline int replace(std::string& str, const std::string& pattern, const std::string& newpat) {
        int count = 0; // 记录替换的次数
        const size_t nsize = newpat.size(); // 新子串的大小
        const size_t psize = pattern.size(); // 旧子串的大小

        // 查找并替换所有匹配的子串
        for (size_t pos = str.find(pattern, 0);
            pos != std::string::npos;
            pos = str.find(pattern, pos + nsize)) {
            str.replace(pos, psize, newpat); // 替换操作
            count++; // 增加计数
        }

        return count; // 返回替换的次数
    }

    // 字符串分割
    // 参数:
    //   s: 需要分割的字符串。
    //   c: 分割符字符。
    //   v: 存储分割结果的向量。
    // 返回值：无，结果存储在 v 中。
    template<typename T>
    inline void split(const std::basic_string<T>& s, T c, std::vector< std::basic_string<T> >& v) {
        if (s.size() == 0) // 如果字符串为空，直接返回
            return;

        // 定义起始和分隔符位置
        typename std::basic_string< T >::size_type i = 0;
        typename std::basic_string< T >::size_type j = s.find(c); // 查找分隔符

        // 循环查找并分割字符串
        while (j != std::basic_string<T>::npos) {
            std::basic_string<T> buf = s.substr(i, j - i); // 提取子串

            if (buf.size() > 0) // 仅当子串不为空时，添加到结果向量
                v.push_back(buf);

            i = ++j; j = s.find(c, j); // 更新起始位置并查找下一个分隔符
        }

        // 处理最后一段（没有分隔符的情况）
        if (j == std::basic_string<T>::npos) {
            std::basic_string<T> buf = s.substr(i, s.length() - i);
            if (buf.size() > 0)
                v.push_back(buf); // 将最后的子串添加到结果向量
        }
    }
}
