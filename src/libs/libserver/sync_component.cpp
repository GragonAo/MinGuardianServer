#include "sync_component.h"
#include "log4_help.h"
#include "packet.h"
#include "component_help.h"

// 解析应用信息
bool AppInfo::Parse(Proto::AppInfoSync proto)
{
    auto pYaml = ComponentHelp::GetYaml();
    this->AppId = proto.app_id(); // 获取应用 ID
    this->AppType = static_cast<APP_TYPE>(proto.app_type()); // 获取应用类型
    this->Online = proto.online(); // 获取在线状态

    auto pConfig = pYaml->GetIPEndPoint(this->AppType, this->AppId); // 获取 IP 和端口配置
    if (pConfig == nullptr)
        return false; // 如果配置为空，返回 false

    this->Ip = pConfig->Ip; // 设置 IP
    this->Port = pConfig->Port; // 设置端口
    return true; // 返回成功
}

// 解析应用信息并更新套接字
void SyncComponent::Parse(Proto::AppInfoSync proto, SOCKET socket)
{
    const auto iter = _apps.find(proto.app_id()); // 查找应用 ID
    if (iter == _apps.end())
    {
        AppInfo syncAppInfo;
        if (syncAppInfo.Parse(proto)) // 解析应用信息
        {
            syncAppInfo.Socket = socket; // 设置套接字
            _apps[syncAppInfo.AppId] = syncAppInfo; // 存储应用信息
        }
    }
    else
    {
        const int appId = proto.app_id();
        _apps[appId].Online = proto.online(); // 更新在线状态
        _apps[appId].Socket = socket; // 更新套接字
    }
}

// 处理应用命令
void SyncComponent::HandleCmdApp(Packet* pPacket)
{
    auto cmdProto = pPacket->ParseToProto<Proto::CmdApp>(); // 解析命令
    auto cmdType = cmdProto.cmd_type();
    if (cmdType == Proto::CmdApp_CmdType_Info)
    {
        CmdShow(); // 显示命令信息
    }
}

// 处理网络断开连接
void SyncComponent::HandleNetworkDisconnect(Packet* pPacket)
{
    SOCKET socket = pPacket->GetSocketKey()->Socket; // 获取套接字
    const auto iter = std::find_if(_apps.begin(), _apps.end(), [&socket](auto pair)
        {
            return pair.second.Socket == socket; // 查找对应的应用
        });

    if (iter == _apps.end())
        return; // 如果未找到，直接返回

    _apps.erase(iter); // 从应用列表中删除
}

// 处理应用信息同步
void SyncComponent::AppInfoSyncHandle(Packet* pPacket)
{
    const auto proto = pPacket->ParseToProto<Proto::AppInfoSync>(); // 解析应用信息
    Parse(proto, pPacket->GetSocketKey()->Socket); // 更新应用信息
}

// 获取指定类型的应用信息
bool SyncComponent::GetOneApp(APP_TYPE appType, AppInfo* pInfo)
{
    if (_apps.empty())
    {
        LOG_ERROR("GetApp failed. no more. appType:" << GetAppName(appType));
        return false; // 如果没有应用，返回错误
    }

    // 查找符合条件的应用
    auto iter = std::find_if(_apps.begin(), _apps.end(), [&appType](auto pair)
        {
            return (pair.second.AppType & appType) != 0; // 检查应用类型
        });

    if (iter == _apps.end())
    {
        LOG_ERROR("GetApp failed. no more. appType:" << appType);
        return false; // 如果未找到，返回错误
    }

    // 查找在线人数最少的应用
    auto min = iter->second.Online;
    int appId = iter->first;
    for (; iter != _apps.end(); ++iter)
    {
        if (min == 0)
            break;

        if ((iter->second.AppType & appType) == 0)
            continue;

        if (iter->second.Online < min)
        {
            min = iter->second.Online; // 更新最小在线人数
            appId = iter->first; // 更新应用 ID
        }
    }

    // 增加在线人数并返回应用信息
    _apps[appId].Online += 1;
    *pInfo = _apps[appId];
    return true; // 返回成功
}

// 显示应用信息
void SyncComponent::CmdShow()
{
    std::stringstream os;
    os << "----" << GetAppName(Global::GetInstance()->GetCurAppType()) << "----\r\n";
    for (auto pair : _apps)
    {
        os << "appId:" << std::setw(4) << pair.first <<
            " type:" << GetAppName(pair.second.AppType) <<
            " online:" << pair.second.Online << "\r\n";
    }

    LOG_DEBUG(os.str().c_str()); // 输出调试信息
}
