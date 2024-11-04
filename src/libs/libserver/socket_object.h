#pragma once
#include "common.h"

#include <log4cplus/streams.h>
#include <ostream>

#include "network_type.h"

// SocketKey 结构，用于标识网络连接
struct SocketKey
{
    // 构造函数
    SocketKey(SOCKET socket, NetworkType netType);

    // 清空 SocketKey 的内容
    void Clear();

    // 从另一个 SocketKey 复制内容
    void CopyFrom(SocketKey* pSocketKey);

    SOCKET Socket;        // 网络套接字
    NetworkType NetType;  // 网络类型

    // 重载不等于运算符
    bool operator != (const SocketKey other) const
    {
        return (Socket != other.Socket) || (NetType != other.NetType);
    };

    // 重载等于运算符
    bool operator == (const SocketKey other) const
    {
        return (Socket == other.Socket) && (NetType == other.NetType);
    };

    static SocketKey None; // 静态成员，表示无效的 SocketKey
};

// TagType 枚举，表示标签类型
enum class TagType
{
    None = Proto::TagType::TagTypeNone,
    Account = Proto::TagType::TagTypeAccount,
    App = Proto::TagType::TagTypeApp,
    Entity = Proto::TagType::TagTypeEntity,
    ToWorld = Proto::TagType::TagTypeToWorld,
    Player = Proto::TagType::TagTypePlayer,
};

// 判断标签类型是否是字符串类型
inline bool IsTagTypeStr(const TagType iType)
{
    return iType == TagType::Account;
}

// 获取标签类型的名称
inline const char* GetTagTypeName(const TagType iType)
{
    switch (iType)
    {
        case TagType::Account: return "Account";
        case TagType::App: return "App";
        case TagType::Entity: return "world";
        case TagType::Player: return "player";
        default: return "None";
    }
}

// TagValue 结构，表示标签的值
struct TagValue
{
    std::string KeyStr{ "" }; // 字符串键
    uint64 KeyInt64{ 0 };     // 整数键

    // 重载不等于运算符
    bool operator != (const TagValue& other) const
    {
        return (KeyStr != other.KeyStr) || (KeyInt64 != other.KeyInt64);
    };

    // 重载等于运算符
    bool operator == (const TagValue& other) const
    {
        return (KeyStr == other.KeyStr) && (KeyInt64 == other.KeyInt64);
    };
};

// TagKey 结构，表示多个标签的集合
struct TagKey
{
public:
    // 清空标签集合
    void Clear();

    // 获取标签映射
    std::map<TagType, TagValue>* GetTags() { return &_tags; }

    // 添加标签
    void AddTag(TagType tagType, std::string value);
    void AddTag(TagType tagType, uint64 value);
    void AddTag(TagType tagType, TagValue value);

    // 获取指定类型的标签值
    TagValue* GetTagValue(TagType tagType);

    // 从另一个 TagKey 复制内容
    void CopyFrom(TagKey* pNetIdentify);

    // 比较两个 TagKey 的标签
    bool CompareTags(TagKey* pIdentify);

protected:
    // 静态方法，比较两个 TagKey 的指定标签
    static bool CompareTags(TagKey* pA, TagKey* pB, TagType tagtype);

protected:
    std::map<TagType, TagValue> _tags; // 存储标签类型和标签值的映射
};

// NetIdentify 结构，表示网络标识
struct NetIdentify
{
public:
    NetIdentify() = default; // 默认构造函数

    // 析构函数，清理资源
    ~NetIdentify()
    {
        _socketKey.Clear();
        _tagKey.Clear();
    }

    SocketKey* GetSocketKey() { return &_socketKey; }
    TagKey* GetTagKey() { return &_tagKey; }

protected:
    SocketKey _socketKey{ INVALID_SOCKET, NetworkType::None }; // 网络套接字标识
    TagKey _tagKey; // 标签标识
};

// 输出运算符重载，输出 TagKey
std::ostream& operator <<(std::ostream& os, TagKey* pTagKey);

// 输出运算符重载，输出 NetIdentify
std::ostream& operator <<(std::ostream& os, NetIdentify* pIdentify);

#if ENGINE_PLATFORM == PLATFORM_WIN32
// log4cplus 输出运算符重载，输出 TagKey
log4cplus::tostream& operator <<(log4cplus::tostream& os, TagKey* pTagKey);

// log4cplus 输出运算符重载，输出 NetIdentify
log4cplus::tostream& operator <<(log4cplus::tostream& os, NetIdentify* pIdentify);
#endif
