#include "log4_help.h"
#include "common.h"
#include <list>

#if ENGINE_PLATFORM == PLATFORM_WIN32

// 设置控制台文本颜色的函数
void SetColor(const int colorEx)
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), colorEx);
}

#endif

#if LOG_MSG_OPEN

// 检查指定的消息ID是否应该显示日志
bool IsLogShowMsg(const int msgId)
{
    std::list<int> lists;

    // 在此处可以添加希望屏蔽的消息ID
    // lists.push_back((int)Proto::MI_Ping);
    // lists.push_back((int)Proto::MI_AppInfoSync);
    // lists.push_back((int)Proto::MI_WorldInfoSyncToGather);
    // lists.push_back((int)Proto::MI_WorldProxyInfoSyncToGather);

    // 检查消息ID是否在屏蔽列表中
    const auto iter = std::find(lists.begin(), lists.end(), msgId);
    if (iter != lists.end())
        return false; // 如果在列表中，返回false，不显示日志

    return true; // 如果不在列表中，返回true，显示日志
}

#endif

// 根据消息ID获取其名称（使用protobuf定义的枚举）
std::string Log4Help::GetMsgIdName(Proto::MsgId msgId)
{
    const google::protobuf::EnumDescriptor* descriptor = Proto::MsgId_descriptor();
    return descriptor->FindValueByNumber(msgId)->name(); // 返回消息ID对应的名称
}

// 根据整型消息ID获取其名称（转换为Proto::MsgId）
std::string Log4Help::GetMsgIdName(int msgId)
{
    return Log4Help::GetMsgIdName((Proto::MsgId)msgId); // 调用上面的函数
}
