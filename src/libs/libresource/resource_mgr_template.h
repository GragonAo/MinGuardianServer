#pragma once

#include "libserver/log4_help.h"
#include "libserver/res_path.h"
#include "libserver/global.h"

#include "resource_base.h"

// 资源管理器模板类
template<class T>
class ResourceManagerTemplate {
public:
    virtual ~ResourceManagerTemplate() {
        _refs.clear(); // 清理资源引用
    }

    // 初始化资源管理器
    bool Initialize(std::string table, ResPath* pResPath);
    virtual bool AfterInit() { return true; } // 可重写的后初始化方法
    T* GetResource(int id); // 根据 ID 获取资源

protected:
    bool ParserHead(std::string line); // 解析头信息
    bool LoadReference(std::string line); // 加载资源引用

protected:
    std::string _cvsName; // CSV 名称
    std::map<std::string, int> _head; // 头部信息映射
    std::map<int, T*> _refs; // 资源引用映射
};

// 初始化方法
template <class T>
bool ResourceManagerTemplate<T>::Initialize(std::string table, ResPath* pResPath)
{
    _cvsName = table; // 保存表名称
    std::string path = pResPath->FindResPath("/resource"); // 找到资源路径
    path = strutil::format("%s/%s.csv", path.c_str(), table.c_str()); // 拼接完整路径
    std::ifstream reader(path.c_str(), std::ios::in); // 打开 CSV 文件
    if (!reader)
    {
        LOG_ERROR("can't open file. " << path.c_str());
        return false; // 打开失败
    }

    LOG_DEBUG("load file. " << path.c_str());

    if (reader.eof())
    {
        LOG_ERROR("read head failed. stream is eof.");
        return false; // 如果文件为空
    }

    std::string line;
    std::getline(reader, line); // 读取第一行（头信息）
    std::transform(line.begin(), line.end(), line.begin(), ::tolower); // 转换为小写

    // 解析头信息
    if (!ParserHead(line))
    {
        LOG_ERROR("parse head failed. " << path.c_str());
        return false; // 解析失败
    }

    // 逐行读取资源数据
    while (true)
    {
        if (reader.eof())
            break;

        std::getline(reader, line);
        if (line.empty())
            continue; // 跳过空行

        std::transform(line.begin(), line.end(), line.begin(), ::tolower); // 转换为小写
        LoadReference(line); // 加载资源引用
    }

    if (!AfterInit())
        return false; // 执行后初始化检查

    return true; // 成功初始化
}

// 根据 ID 获取资源
template <class T>
T* ResourceManagerTemplate<T>::GetResource(int id)
{
    auto iter = _refs.find(id);
    if (iter == _refs.end())
        return nullptr; // 资源未找到

    return _refs[id]; // 返回找到的资源
}

// 解析头信息
template <class T>
bool ResourceManagerTemplate<T>::ParserHead(std::string line)
{
    if (line.empty())
        return false; // 行为空则返回失败

    std::vector<std::string> propertyList = ResourceBase::ParserLine(line); // 解析行

    // 将属性名和索引存入头信息映射
    for (size_t i = 0; i < propertyList.size(); i++)
    {
        _head.insert(std::make_pair(propertyList[i], i));
    }

    return true; // 解析成功
}

// 加载资源引用
template <class T>
bool ResourceManagerTemplate<T>::LoadReference(std::string line)
{
    auto pT = new T(_head); // 创建资源对象
    if (pT->LoadProperty(line) && pT->Check()) // 加载属性并检查有效性
    {
        _refs.insert(std::make_pair(pT->GetId(), pT)); // 存入引用映射
        return true; // 成功加载
    }

    return false; // 加载失败
}
