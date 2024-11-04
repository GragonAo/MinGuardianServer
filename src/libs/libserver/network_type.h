#pragma once

// 定义网络类型的枚举类，用于区分不同的网络连接类型
enum class NetworkType
{
    None = 0,             // 无网络类型
    TcpListen = 1 << 0,   // TCP 监听模式，表示服务器监听连接
    TcpConnector = 1 << 1,// TCP 连接模式，表示客户端发起连接

    HttpListen = 1 << 2,  // HTTP 监听模式，服务器端监听 HTTP 请求
    HttpConnector = 1 << 3,// HTTP 连接模式，客户端发起 HTTP 请求
};

// 返回给定的网络类型的名称字符串
// @param appType - 网络类型的枚举值
// @return - 网络类型名称的 C 字符串
inline const char* GetNetworkTypeName(const NetworkType appType)
{
    // 根据传入的网络类型返回对应的名称
    if (appType == NetworkType::TcpListen)
        return "TcpListen";  // 如果是 TCP 监听模式，返回 "TcpListen"
    else if (appType == NetworkType::TcpConnector)
        return "TcpConnector";  // 如果是 TCP 连接模式，返回 "TcpConnector"
    else if (appType == NetworkType::HttpListen)
        return "HttpListen";  // 如果是 HTTP 监听模式，返回 "HttpListen"
    else
        return "HttpConnector";  // 如果是其他类型，返回 "HttpConnector"（默认为 HTTP 连接模式）
}
