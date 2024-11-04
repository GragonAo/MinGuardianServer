#include "network_locator.h"
#include "network_listen.h"
#include <algorithm>
#include <utility>
#include "message_system_help.h"
#include "component_help.h"
#include "socket_object.h"
#include "global.h"
#include "message_system.h"

// 初始化
void NetworkLocator::Awake()
{
    std::lock_guard<std::mutex> g1(_lock);

    // 清空现有的网络标识、连接器和监听器
    _netIdentify.clear();
    _connectors.clear();
    _listens.clear();

    // 消息系统
    auto pMsgSystem = GetSystemManager()->GetMessageSystem();

    // 注册消息处理函数
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_AppRegister, BindFunP1(this, &NetworkLocator::HandleAppRegister));
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_NetworkDisconnect, BindFunP1(this, &NetworkLocator::HandleNetworkDisconnect));
}

// 返回池中
void NetworkLocator::BackToPool()
{
    std::lock_guard<std::mutex> g1(_lock);

    // 清空网络标识、连接器和监听器
    _netIdentify.clear();
    _connectors.clear();
    _listens.clear();
}

// 添加连接器定位
void NetworkLocator::AddConnectorLocator(INetwork* pNetwork, NetworkType networkType)
{
    std::lock_guard<std::mutex> guard(_lock);
    _connectors[networkType] = pNetwork;
}

// 添加网络标识
void NetworkLocator::AddNetworkIdentify(SocketKey* pSocket, uint64 appKey)
{
    _lock.lock();
    const auto iter = _netIdentify.find(appKey);
    if (iter != _netIdentify.end())
    {
        LOG_WARN("connector locator recv multiple socket.");
        _netIdentify.erase(appKey); // 如果已经存在，移除旧的标识
    }

    NetIdentify netIdentify;
    netIdentify.GetSocketKey()->CopyFrom(pSocket); // 复制socket信息
    netIdentify.GetTagKey()->AddTag(TagType::App, appKey); // 添加应用标识
    _netIdentify.insert(std::make_pair(appKey, netIdentify)); // 插入新标识
    _lock.unlock();

    auto appType = GetTypeFromAppKey(appKey);
    auto appId = GetIdFromAppKey(appKey);
    LOG_DEBUG("connected appType:" << GetAppName(appType) << " appId:" << appId << " " << &netIdentify);

    const auto pGlobal = Global::GetInstance();
    // 发送应用注册消息
    if ((pGlobal->GetCurAppType() & APP_APPMGR) == 0)
    {
        if (((appType & APP_APPMGR) != 0) || ((appType & APP_SPACE) != 0) || ((appType & APP_GAME) != 0))
        {
            Proto::AppRegister proto;
            proto.set_type(pGlobal->GetCurAppType());
            proto.set_id(pGlobal->GetCurAppId());
            MessageSystemHelp::SendPacket(Proto::MsgId::MI_AppRegister, proto, appType, appId);
        }
    }
}

// 获取指定应用类型的网络
std::list<NetIdentify> NetworkLocator::GetAppNetworks(const APP_TYPE appType)
{
    std::lock_guard<std::mutex> guard(_lock);
    std::list<NetIdentify> rs;

    auto iter = _netIdentify.begin();
    while (iter != _netIdentify.end()) {
        iter = std::find_if(iter, _netIdentify.end(), [appType](auto pair)
            {
                auto appKey = pair.first;
                return GetTypeFromAppKey(appKey) == appType; // 匹配应用类型
            });

        if (iter != _netIdentify.end())
        {
            rs.emplace_back(iter->second); // 添加到结果列表
            ++iter;
        }
    }

    return rs; // 返回结果列表
}

// 获取指定应用的网络标识
NetIdentify NetworkLocator::GetNetworkIdentify(const APP_TYPE appType, const int appId)
{
    std::lock_guard<std::mutex> guard(_lock);

    const auto appKey = GetAppKey(appType, appId); // 生成应用键
    const auto iter = _netIdentify.find(appKey);
    if (iter == _netIdentify.end())
        return NetIdentify(); // 如果未找到，返回空标识

    return iter->second; // 返回找到的网络标识
}

// 添加监听器定位
void NetworkLocator::AddListenLocator(INetwork* pNetwork, NetworkType networkType)
{
    std::lock_guard<std::mutex> guard(_lock);
    _listens[networkType] = pNetwork; // 插入监听器
}

// 根据网络类型获取网络接口
INetwork* NetworkLocator::GetNetwork(NetworkType networkType)
{
    std::lock_guard<std::mutex> guard(_lock);

    // 查找监听器
    if (networkType == NetworkType::HttpListen || networkType == NetworkType::TcpListen)
    {
        auto iter = _listens.find(networkType);
        if (iter == _listens.end())
            return nullptr; // 未找到

        return iter->second; // 返回监听器
    }

    // 查找连接器
    if (networkType == NetworkType::HttpConnector || networkType == NetworkType::TcpConnector)
    {
        auto iter = _connectors.find(networkType);
        if (iter == _connectors.end())
            return nullptr; // 未找到

        return iter->second; // 返回连接器
    }

    return nullptr; // 默认返回空
}

// 处理应用注册
void NetworkLocator::HandleAppRegister(Packet* pPacket)
{
    std::lock_guard<std::mutex> guard(_lock);

    const auto proto = pPacket->ParseToProto<Proto::AppRegister>(); // 解析应用注册包
    const uint64 appKey = GetAppKey(proto.type(), proto.id()); // 获取应用键

    const auto iter = _netIdentify.find(appKey);
    if (iter == _netIdentify.end())
    {
        NetIdentify netIdentify;
        netIdentify.GetSocketKey()->CopyFrom(pPacket->GetSocketKey()); // 复制socket信息
        netIdentify.GetTagKey()->AddTag(TagType::App, appKey); // 添加应用标识
        _netIdentify.insert(std::make_pair(appKey, netIdentify)); // 插入新标识

        LOG_DEBUG("connected appType:" << GetAppName(GetTypeFromAppKey(appKey)) << " appId:" << GetIdFromAppKey(appKey) << " " << &netIdentify);

        // 触发网络监听键的消息
        MessageSystemHelp::DispatchPacket(Proto::MsgId::MI_NetworkListenKey, &netIdentify);
    } 
    else
    {
        _netIdentify[appKey].GetTagKey()->CopyFrom(pPacket->GetTagKey()); // 更新现有标识的标签
    }
}

// 处理网络断开连接
void NetworkLocator::HandleNetworkDisconnect(Packet* pPacket)
{
    std::lock_guard<std::mutex> guard(_lock);

    auto pTags = pPacket->GetTagKey();
    auto pTagApp = pTags->GetTagValue(TagType::App);
    if (pTagApp == nullptr)
        return;

    auto appKey = pTagApp->KeyInt64;
    _netIdentify.erase(appKey); // 移除网络标识

    LOG_DEBUG("remove appType:" << GetAppName(GetTypeFromAppKey(appKey)) << " appId:" << GetIdFromAppKey(appKey));

    // 重新创建连接
    auto appType = GetTypeFromAppKey(appKey);
    auto appId = GetIdFromAppKey(appKey);
    const auto pYaml = ComponentHelp::GetYaml();
    const auto pCommonConfig = pYaml->GetIPEndPoint(appType, appId);

    TagValue tagValue{ "", appKey };
    MessageSystemHelp::CreateConnect(NetworkType::TcpConnector, TagType::App, tagValue, pCommonConfig->Ip, pCommonConfig->Port);
}
