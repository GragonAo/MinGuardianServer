#pragma once // 防止重复包含该头文件

#include "network.h" // 包含网络基础类

// 定义 NetworkListen 类，继承自 Network 类并实现两个 IAwakeSystem 接口
class NetworkListen : public Network,
    virtual public IAwakeSystem<std::string, int>, // 实现以 (std::string, int) 参数的初始化接口
    virtual public IAwakeSystem<int, int> // 实现以 (int, int) 参数的初始化接口
{
public:
    // 使用 IP 和端口初始化网络监听器
    void Awake(std::string ip, int port) override;

    // 使用应用类型和 ID 初始化网络监听器
    void Awake(int appType, int appId) override;

    // 使用 IP、端口和网络类型初始化网络监听器
    void Awake(std::string ip, int port, NetworkType iType);

    // 将网络监听器返回到对象池
    void BackToPool() override;

    // 更新网络监听器状态
    virtual void Update();

    // 获取类的类型名称
    const char* GetTypeName() override;

    // 获取类的类型哈希码
    uint64 GetTypeHashCode() override;

    // 显示当前 socket 状态
    void CmdShow();

private:
    // 处理网络监听键的消息
    void HandleListenKey(Packet* pPacket);

protected:
    // 接受客户端连接
    virtual int Accept();

#ifdef EPOLL
    // epoll 事件处理
    virtual void OnEpoll(SOCKET fd, int index) override;
#endif

private:
#ifdef EPOLL
    int _mainSocketEventIndex{ -1 }; // 主 socket 事件索引
#endif

    SOCKET _masterSocket{ INVALID_SOCKET }; // 主监听 socket，初始状态为无效
};
