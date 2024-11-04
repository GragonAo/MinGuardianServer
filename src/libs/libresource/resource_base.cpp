#include "resource_base.h"
#include "libserver/log4_help.h"
#include "libserver/util_string.h"

std::vector<std::string> ResourceBase::ParserLine(std::string line)
{
    line = strutil::trim(line); // 去除行首尾空白

    std::vector<std::string> propertyList; // 属性列表
    bool isQuoted = false; // 用于判断当前属性是否被引号包围
    int index = 0;

    do
    {
        // 判断行的首字符是否为引号
        if (line.at(0) == '\"')
            isQuoted = true;
        else
            isQuoted = false;

        // 根据是否被引号包围选择查找的位置
        if (isQuoted)
        {
            line.erase(0, 1); // 去掉开头的引号
            index = line.find('\"'); // 查找结束引号的位置
        }
        else
        {
            index = line.find(','); // 查找逗号的位置
        }

        if (index > 0)
        {
            propertyList.push_back(line.substr(0, index)); // 提取属性
            line = line.erase(0, index + 1); // 更新行内容，去掉已处理部分
        }
        else
        {
            propertyList.push_back(line); // 添加最后一个属性
            break;
        }

        if (line.empty())
            break;

    } while (true);

    return propertyList; // 返回属性列表
}

bool ResourceBase::LoadProperty(const std::string line)
{
    std::vector<std::string> propertyList = ParserLine(line); // 解析属性行

    // 检查属性数量是否符合要求
    if (propertyList.size() < _head.size())
    {
        LOG_ERROR("LoadProperty failed. " << "line size:" << propertyList.size() << " head size:" << _head.size() << " \t" << line.c_str());
        return false;
    }

    // 处理每个属性并去除空白
    for (size_t i = 0; i < propertyList.size(); i++)
    {
        _props.push_back(strutil::trim(propertyList[i]));
    }

    _id = std::stoi(_props[0]); // 将第一项作为 ID

    GenStruct(); // 调用纯虚函数生成结构
    return true;
}

void ResourceBase::DebugHead() const
{
    // 调试输出属性头部信息
    for (auto one : _head)
    {
        LOG_DEBUG("head name:[" << one.first.c_str() << "] head index:" << one.second);
    }
}

bool ResourceBase::GetBool(std::string name)
{
    const auto iter = _head.find(name); // 查找属性名称
    if (iter == _head.end())
    {
        LOG_ERROR("GetInt Failed. id:" << _id << " name:[" << name.c_str() << "]");
        DebugHead(); // 输出调试信息
        return false;
    }

    return std::stoi(_props[iter->second]) == 1; // 将字符串转换为布尔值
}

int ResourceBase::GetInt(std::string name)
{
    const auto iter = _head.find(name); // 查找属性名称
    if (iter == _head.end())
    {
        LOG_ERROR("GetInt Failed. id:" << _id << " name:[" << name.c_str() << "]");
        DebugHead(); // 输出调试信息
        return 0;
    }

    return std::stoi(_props[iter->second]); // 返回整型值
}

std::string ResourceBase::GetString(std::string name)
{
    const auto iter = _head.find(name); // 查找属性名称
    if (iter == _head.end())
    {
        LOG_ERROR("GetString Failed. id:" << _id << " name:[" << name.c_str() << "]");
        DebugHead(); // 输出调试信息
        return "";
    }

    return _props[iter->second]; // 返回字符串值
}
