#include <iostream>

#include "common.h"
#include "network_connector.h"
#include "network_locator.h"
#include "log4_help.h"
#include "yaml.h"
#include "thread_mgr.h"
#include "update_component.h"
#include "component_help.h"
#include "message_system.h"

void NetworkConnector::Awake(int iType, int mixConnectAppType)
{
    _networkType = (NetworkType)iType;

    // 获取网络定位器并注册连接器
    auto pNetworkLocator = ThreadMgr::GetInstance()->GetEntitySystem()->GetComponent<NetworkLocator>();
    pNetworkLocator->AddConnectorLocator(this, _networkType);

    // 添加更新组件
    AddComponent<UpdateComponent>(BindFunP0(this, &NetworkConnector::Update));

    // 注册消息处理
    auto pMsgSystem = GetSystemManager()->GetMessageSystem();
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_NetworkConnect, BindFunP1(this, &NetworkConnector::HandleNetworkConnect));
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_NetworkRequestDisconnect, BindFunP1(this, &NetworkConnector::HandleDisconnect));

#ifdef EPOLL
    std::cout << "epoll model. connector:" << GetNetworkTypeName(_networkType) << std::endl;
    InitEpoll();
#else
    std::cout << "select model. connector:" << GetNetworkTypeName(_networkType) << std::endl;
#endif

    // 根据混合连接应用类型创建连接
    if (_networkType == NetworkType::TcpConnector && mixConnectAppType > 0)
    {
        const auto pYaml = ComponentHelp::GetYaml();
        APP_TYPE appType;

        // 处理不同的应用类型
        if ((mixConnectAppType & APP_APPMGR) != 0)
        {
            appType = APP_APPMGR;
            const auto pCommonConfig = pYaml->GetIPEndPoint(appType, 0);
            CreateConnector(appType, 0, pCommonConfig->Ip, pCommonConfig->Port);
        }

        if ((mixConnectAppType & APP_DB_MGR) != 0)
        {
            appType = APP_DB_MGR;
            const auto pCommonConfig = pYaml->GetIPEndPoint(appType, 0);
            CreateConnector(appType, 0, pCommonConfig->Ip, pCommonConfig->Port);
        }

        if ((mixConnectAppType & APP_LOGIN) != 0)
        {
            appType = APP_LOGIN;
            auto pLoginConfig = dynamic_cast<LoginConfig*>(pYaml->GetConfig(appType));
            for (auto one : pLoginConfig->Apps)
            {
                CreateConnector(appType, one.Id, one.Ip, one.Port);
            }
        }

        if ((mixConnectAppType & APP_SPACE) != 0)
        {
            appType = APP_SPACE;
            auto pLoginConfig = dynamic_cast<SpaceConfig*>(pYaml->GetConfig(appType));
            for (auto one : pLoginConfig->Apps)
            {
                CreateConnector(appType, one.Id, one.Ip, one.Port);
            }
        }
    }
}

void NetworkConnector::CreateConnector(APP_TYPE appType, int appId, std::string ip, int port)
{
    // 添加连接详细信息
    _connecting.AddObj(new ConnectDetail(TagType::App, TagValue{ "", GetAppKey(appType, appId) }, ip, port));
}

const char* NetworkConnector::GetTypeName()
{
    return typeid(NetworkConnector).name(); // 返回类型名称
}

uint64 NetworkConnector::GetTypeHashCode()
{
    return typeid(NetworkConnector).hash_code(); // 返回类型哈希值
}

bool NetworkConnector::Connect(ConnectDetail* pDetail)
{
    const int socket = CreateSocket(); // 创建socket
    if (socket == INVALID_SOCKET)
        return false;

    sockaddr_in addr;
    memset(&addr, 0, sizeof(sockaddr_in)); // 清零地址结构
    addr.sin_family = AF_INET; // IPv4
    addr.sin_port = htons(pDetail->Port); // 端口
    ::inet_pton(AF_INET, pDetail->Ip.c_str(), &addr.sin_addr.s_addr); // 转换IP地址

    const int rs = ::connect(socket, (struct sockaddr*)&addr, sizeof(sockaddr)); // 发起连接
    if (rs == 0)
    {
        return CreateConnectObj(socket, pDetail->TType, pDetail->TValue, ConnectStateType::Connected);
    }
    else
    {
        const auto socketError = _sock_err(); // 获取socket错误
        if (!NetworkHelp::IsError(socketError))
        {
            // 连接尚未完成，正在等待
#ifdef LOG_TRACE_COMPONENT_OPEN
            std::stringstream traceMsg;
            traceMsg << "create connect != 0 waiting, err=" << socketError;
            traceMsg << " network type:" << GetNetworkTypeName(_networkType);
            ComponentHelp::GetTraceComponent()->Trace(TraceType::Connector, socket, traceMsg.str());
#endif

            return CreateConnectObj(socket, pDetail->TType, pDetail->TValue, ConnectStateType::Connecting);
        }
        else
        {
            // 连接失败
            LOG_WARN("failed to connect 2. ip:" << pDetail->Ip.c_str() << " port:" << pDetail->Port << " network sn:" << _sn << " socket:" << socket << " err:" << socketError);
            _sock_close(socket);
            return false;
        }
    }
}

void NetworkConnector::HandleNetworkConnect(Packet* pPacket)
{
    auto proto = pPacket->ParseToProto<Proto::NetworkConnect>();
    if (proto.network_type() != (int)_networkType)
        return;

    const auto protoTag = proto.tag();
    const auto tagValue = protoTag.tag_value();
    auto tagObj = TagValue{ tagValue.value_str().c_str(), tagValue.value_int64() };
    const auto pObj = new ConnectDetail((TagType)protoTag.tag_type(), tagObj, proto.ip(), proto.port());
    _connecting.AddObj(pObj); // 添加连接详细信息
}

#ifdef EPOLL

void NetworkConnector::Update()
{
    // 更新连接状态
    if (_connecting.CanSwap())
        _connecting.Swap(nullptr);

    if (!_connecting.GetReaderCache()->empty())
    {
        auto pReader = _connecting.GetReaderCache();
        for (auto iter = pReader->begin(); iter != pReader->end(); ++iter)
        {
            if (Connect(iter->second))
            {
                _connecting.RemoveObj(iter->first); // 移除已连接对象
            }
        }
    }

    Epoll(); // 处理epoll事件
    OnNetworkUpdate(); // 更新网络状态
}

#else

void NetworkConnector::Update()
{
#if LOG_TRACE_COMPONENT_OPEN
    CheckBegin();
#endif

    // 更新连接状态
    if (_connecting.CanSwap())
        _connecting.Swap(nullptr);

    if (!_connecting.GetReaderCache()->empty())
    {
        auto pReader = _connecting.GetReaderCache();
        for (auto iter = pReader->begin(); iter != pReader->end(); ++iter)
        {
            if (Connect(iter->second))
            {
                _connecting.RemoveObj(iter->first); // 移除已连接对象
            }
        }
    }

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);

    _fdMax = 0;

    Select(); // 处理select事件
    OnNetworkUpdate(); // 更新网络状态

#if LOG_TRACE_COMPONENT_OPEN
    CheckPoint(GetNetworkTypeName(_networkType));
#endif
}

#endif
