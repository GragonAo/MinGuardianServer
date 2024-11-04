#include "socket_locator.h"
#include "message_system.h"

void SocketLocator::Awake()
{
    auto pMsgSystem = GetSystemManager()->GetMessageSystem();
    // 注册网络断开消息的处理函数
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_NetworkDisconnect, BindFunP1(this, &SocketLocator::HandleNetworkDisconnect));
}

void SocketLocator::BackToPool()
{
    // 清空组件映射，准备对象返回池中
    _componentes.clear();
}

void SocketLocator::RegisterToLocator(SOCKET socket, uint64 sn)
{
    std::lock_guard<std::mutex> guard(_lock); // 确保线程安全
    const auto iter = _componentes.find(socket);
    if (iter != _componentes.end())
    {
        // 如果套接字已存在，更新其序列号
        _componentes[socket] = sn;
    }
    else
    {
        // 插入新的套接字和序列号
        _componentes.insert(std::make_pair(socket, sn));
    }
}

void SocketLocator::Remove(const SOCKET socket)
{
    std::lock_guard<std::mutex> guard(_lock); // 确保线程安全
    // 从映射中移除套接字
    _componentes.erase(socket);
}

uint64 SocketLocator::GetTargetEntitySn(const SOCKET socket)
{
    std::lock_guard<std::mutex> guard(_lock); // 确保线程安全
    const auto iter = _componentes.find(socket);
    if (iter == _componentes.end())
        return 0; // 如果未找到，返回0

    return iter->second; // 返回对应的序列号
}

void SocketLocator::HandleNetworkDisconnect(Packet* pPacket)
{
    std::lock_guard<std::mutex> guard(_lock); // 确保线程安全
    // 处理网络断开，移除对应的套接字
    _componentes.erase(pPacket->GetSocketKey()->Socket);
}
