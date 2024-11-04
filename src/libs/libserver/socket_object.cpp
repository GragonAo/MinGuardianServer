#include "socket_object.h"

// 初始化静态成员 SocketKey::None
SocketKey SocketKey::None = SocketKey(INVALID_SOCKET, NetworkType::None);

// SocketKey 构造函数
SocketKey::SocketKey(SOCKET socket, NetworkType netType)
{
    Socket = socket; // 设置套接字
    NetType = netType; // 设置网络类型
}

// 清空套接字信息
void SocketKey::Clear()
{
    Socket = INVALID_SOCKET; // 将套接字设置为无效
    NetType = NetworkType::None; // 设置网络类型为无
}

// 从另一个 SocketKey 复制信息
void SocketKey::CopyFrom(SocketKey* pSocketKey)
{
    Socket = pSocketKey->Socket; // 复制套接字
    NetType = pSocketKey->NetType; // 复制网络类型
}

// 清空标签
void TagKey::Clear()
{
    _tags.clear(); // 清空标签映射
}

// 添加字符串类型的标签
void TagKey::AddTag(TagType tagType, std::string value)
{
    const auto iter = _tags.find(tagType);
    if (iter == _tags.end())
    {
        _tags[tagType] = TagValue{ value, 0L }; // 如果标签不存在，则添加
    }
    else
    {
        _tags[tagType].KeyStr = value; // 更新标签的字符串值
        _tags[tagType].KeyInt64 = 0; // 清空整数值
    }
}

// 添加整数类型的标签
void TagKey::AddTag(TagType tagType, uint64 value)
{
    const auto iter = _tags.find(tagType);
    if (iter == _tags.end())
    {
        _tags[tagType] = TagValue{ "", value }; // 如果标签不存在，则添加
    }
    else
    {
        _tags[tagType].KeyStr = ""; // 清空字符串值
        _tags[tagType].KeyInt64 = value; // 更新标签的整数值
    }
}

// 添加 TagValue 类型的标签
void TagKey::AddTag(TagType tagType, TagValue value)
{
    if (value.KeyInt64 != 0)
        AddTag(tagType, value.KeyInt64); // 如果整数值不为0，添加整数标签
    else
        AddTag(tagType, value.KeyStr); // 否则，添加字符串标签
}

// 获取指定类型的标签值
TagValue* TagKey::GetTagValue(TagType tagType)
{
    if (_tags.find(tagType) == _tags.end())
        return nullptr; // 如果标签不存在，返回 nullptr

    return &(_tags[tagType]); // 返回标签值的地址
}

// 从另一个 TagKey 复制标签
void TagKey::CopyFrom(TagKey* pNetIdentify)
{
    auto tags = pNetIdentify->GetTags(); // 获取标签映射
    for (auto iter = tags->begin(); iter != tags->end(); ++iter)
    {
        AddTag(iter->first, iter->second); // 逐个添加标签
    }
}

// 比较标签
bool TagKey::CompareTags(TagKey* pIdentify)
{
    if (!CompareTags(this, pIdentify, TagType::Account))
        return false;

    if (!CompareTags(this, pIdentify, TagType::App))
        return false;

    return true; // 所有标签均相同
}

// 静态比较标签
bool TagKey::CompareTags(TagKey* pA, TagKey* pB, TagType tagtype)
{
    TagValue* pTagValueA = pA->GetTagValue(tagtype);
    TagValue* pTagValueB = pB->GetTagValue(tagtype);
    if (pTagValueA != nullptr && pTagValueB != nullptr)
    {
        if (IsTagTypeStr(tagtype))
            return pTagValueA->KeyStr == pTagValueB->KeyStr; // 比较字符串标签
        else
            return pTagValueA->KeyInt64 == pTagValueB->KeyInt64; // 比较整数标签
    }

    return true; // 标签不存在视为相等
}

// 输出 TagKey 的信息
std::ostream& operator<<(std::ostream& os, TagKey* pTagKey)
{
    auto tags = pTagKey->GetTags(); // 获取标签映射
    for (auto iter = tags->begin(); iter != tags->end(); ++iter)
    {
        auto typeName = GetTagTypeName(iter->first); // 获取标签名称
        os << " tag:" << typeName << " v:"; // 输出标签信息

        if (IsTagTypeStr(iter->first))
            os << iter->second.KeyStr.c_str(); // 输出字符串值
        else
            os << iter->second.KeyInt64; // 输出整数值
    }

    return os; // 返回输出流
}

// 输出 NetIdentify 的信息
std::ostream& operator<<(std::ostream& os, NetIdentify* pIdentify)
{
    os << " socket:" << pIdentify->GetSocketKey()->Socket
        << " networkType:" << GetNetworkTypeName(pIdentify->GetSocketKey()->NetType); // 输出套接字和网络类型

    auto tags = pIdentify->GetTagKey(); // 获取标签
    os << tags; // 输出标签信息

    return os; // 返回输出流
}

#if ENGINE_PLATFORM == PLATFORM_WIN32
// 在 Windows 平台上重载输出 TagKey 的信息
log4cplus::tostream& operator<<(log4cplus::tostream& os, TagKey* pTagKey)
{
    auto tags = pTagKey->GetTags(); // 获取标签映射
    for (auto iter = tags->begin(); iter != tags->end(); ++iter)
    {
        auto typeName = GetTagTypeName(iter->first); // 获取标签名称
        os << " tag:" << typeName << " v:"; // 输出标签信息

        if (IsTagTypeStr(iter->first))
            os << iter->second.KeyStr.c_str(); // 输出字符串值
        else
            os << iter->second.KeyInt64; // 输出整数值
    }

    return os; // 返回输出流
}

// 在 Windows 平台上重载输出 NetIdentify 的信息
log4cplus::tostream& operator<<(log4cplus::tostream& os, NetIdentify* pIdentify)
{
    os << " socket:" << pIdentify->GetSocketKey()->Socket
        << " networkType:" << GetNetworkTypeName(pIdentify->GetSocketKey()->NetType); // 输出套接字和网络类型

    auto tags = pIdentify->GetTagKey(); // 获取标签
    os << tags; // 输出标签信息

    return os; // 返回输出流
}
#endif
