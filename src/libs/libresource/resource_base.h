#pragma once

#include <vector>
#include <map>
#include <string>

class ResourceBase
{
public:
    virtual ~ResourceBase() = default; // 虚析构函数，确保派生类正确析构
    explicit ResourceBase(std::map<std::string, int>& head) : _id(0), _head(head) {}

    int GetId() const { return _id; } // 获取资源的 ID

    bool LoadProperty(const std::string line); // 加载资源属性

    // 纯虚函数，要求派生类实现检查逻辑
    virtual bool Check() = 0;

    // 解析一行字符串并返回属性列表
    static std::vector<std::string> ParserLine(std::string line);

protected:
    // 纯虚函数，要求派生类实现结构生成逻辑
    virtual void GenStruct() = 0;

    // 根据属性名称获取字符串、布尔值和整型值
    std::string GetString(std::string name);
    bool GetBool(std::string name);
    int GetInt(std::string name);

    void DebugHead() const; // 调试输出头部信息

private:
    int _id; // 资源的 ID
    std::map<std::string, int>& _head; // 属性头部信息的映射
    std::vector<std::string> _props; // 资源的属性列表
};
