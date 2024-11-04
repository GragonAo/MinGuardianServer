#include "global.h"

#if ENGINE_PLATFORM != PLATFORM_WIN32
#include <sys/time.h>
#include <uuid/uuid.h>
#else
#include <objbase.h>
#endif

Global::Global(APP_TYPE appType, int appId)
{
    _appType = appType;
    _appId = appId;
    std::cout << "app type:" << GetAppName(appType) << " id:" << _appId << std::endl;

    UpdateTime(); // 初始化时更新时间
}

int Global::GetAppIdFromSN(uint64 sn)
{
    sn = sn << 38; // 左移38位
    sn = sn >> 38; // 再右移38位，提取应用ID
    sn = sn >> 16; // 再右移16位，获得SN的应用ID部分
    return static_cast<int>(sn);
}

uint64 Global::GenerateSN()
{
    std::lock_guard<std::mutex> guard(_mtx); // 线程安全锁
    // (38, 10, 16) 组合时间、应用ID和票据生成SN
    const uint64 ret = ((TimeTick >> 10) << 26) + (_appId << 16) + _snTicket;
    _snTicket += 1; // 增加票据
    return ret; // 返回生成的SN
}

APP_TYPE Global::GetCurAppType() const
{
    return _appType; // 返回当前应用类型
}

int Global::GetCurAppId() const
{
    return _appId; // 返回当前应用ID
}

void Global::UpdateTime()
{
#if ENGINE_PLATFORM != PLATFORM_WIN32
    struct timeval tv;
    gettimeofday(&tv, nullptr); // 获取系统时间
    TimeTick = tv.tv_sec * 1000 + tv.tv_usec * 0.001; // 转换为毫秒
#else
    auto timeValue = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    TimeTick = timeValue.time_since_epoch().count(); // 获取自纪元以来的毫秒数
#endif
}

std::string Global::GenerateUUID()
{
#if ENGINE_PLATFORM == PLATFORM_WIN32
    char buf[64] = { 0 };
    GUID guid;
    if (S_OK == ::CoCreateGuid(&guid)) // 创建GUID
    {
        _snprintf_s(buf, sizeof(buf),
            "%08X-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X",
            guid.Data1,
            guid.Data2,
            guid.Data3,
            guid.Data4[0], guid.Data4[1],
            guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5],
            guid.Data4[6], guid.Data4[7]
        );
    }

    std::string tokenkey = buf; // 转换为字符串
#else
    uuid_t uuid;
    uuid_generate(uuid); // 生成UUID

    char key[36];
    uuid_unparse(uuid, key); // 将UUID转换为字符串

    std::string tokenkey = key;
#endif

    return tokenkey; // 返回生成的UUID
}
