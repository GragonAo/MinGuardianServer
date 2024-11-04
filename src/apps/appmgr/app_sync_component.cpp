#include "app_sync_component.h"
#include "libserver/message_system_help.h"
#include "libserver/message_system.h"
#include "libserver/network_help.h"

void AppSyncComponent::Awake()
{
    // 定时同步游戏信息到登录服务，每 2 秒执行一次
    AddTimer(0, 2, false, 0, BindFunP0(this, &AppSyncComponent::SyncGameInfoToLogin));

    Json::StreamWriterBuilder jsonBuilder;
    _jsonWriter = jsonBuilder.newStreamWriter();

    // 获取消息系统实例并注册消息处理器
    auto pMsgSystem = GetSystemManager()->GetMessageSystem();

    // 注册处理 HTTP 登录请求的处理器
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_HttpRequestLogin, BindFunP1(this, &AppSyncComponent::HandleHttpRequestLogin));

    // 注册应用信息同步处理器
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_AppInfoSync, BindFunP1(this, &AppSyncComponent::HandleAppInfoSync));

    // 注册命令处理器
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_CmdApp, BindFunP1(this, &AppSyncComponent::HandleCmdApp));

    // 注册处理网络断开的处理器
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_NetworkDisconnect, BindFunP1(this, &AppSyncComponent::HandleNetworkDisconnect));
}

void AppSyncComponent::BackToPool()
{
    delete _jsonWriter;  // 释放 JSON 写入器
    _apps.clear();       // 清空存储的应用信息
}

void AppSyncComponent::HandleHttpRequestLogin(Packet* pPacket)
{
    Json::Value responseObj;
    AppInfo info;

    // 尝试获取登录应用信息
    if (!GetOneApp(APP_LOGIN, &info))
    {
        // 未找到登录应用信息时的响应
        responseObj["returncode"] = (int)Proto::LoginHttpReturnCode::LHRC_NOTFOUND;
        responseObj["ip"] = "";
        responseObj["port"] = 0;
    }
    else
    {
        // 找到登录应用信息时的响应
        responseObj["returncode"] = (int)Proto::LoginHttpReturnCode::LHRC_OK;
        responseObj["ip"] = info.Ip;
        responseObj["port"] = info.Port;
    }

    std::stringstream jsonStream;
    _jsonWriter->write(responseObj, &jsonStream);

    // 发送 HTTP 响应
    MessageSystemHelp::SendHttpResponse(pPacket, jsonStream.str().c_str(), jsonStream.str().length());
}

void AppSyncComponent::HandleAppInfoSync(Packet* pPacket)
{
    // 调用同步处理函数以更新应用信息
    AppInfoSyncHandle(pPacket);
}

void AppSyncComponent::HandleNetworkDisconnect(Packet* pPacket)
{
    if (!NetworkHelp::IsTcp(pPacket->GetSocketKey()->NetType))
        return;

    // 调用基类的网络断开处理
    SyncComponent::HandleNetworkDisconnect(pPacket);

    // 当网络断开时，更新并同步游戏信息到登录服务
    SyncGameInfoToLogin();
}

void AppSyncComponent::SyncGameInfoToLogin()
{
    Proto::AppInfoListSync proto;

    // 遍历存储的应用信息
    for (auto pair : _apps)
    {
        auto info = pair.second;
        // 如果应用不是游戏类型，则跳过
        if ((info.AppType & APP_GAME) == 0)
            continue;

        // 将游戏应用信息添加到同步列表
        auto pProto = proto.add_apps();
        pProto->set_app_id(info.AppId);
        pProto->set_app_type(info.AppType);
        pProto->set_online(info.Online);
    }

    // 如果有需要同步的信息，发送给登录应用
    if (proto.apps_size() > 0)
    {
        MessageSystemHelp::SendPacketToAllApp(Proto::MsgId::MI_AppInfoListSync, proto, APP_LOGIN);
    }
}
