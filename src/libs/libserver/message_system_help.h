#pragma once
#include "app_type.h"
#include "common.h"
#include "packet.h"

// 前向声明
enum class NetworkType; // 网络类型枚举，定义网络连接的不同类型
class INetwork; // 网络接口类，用于处理不同类型的网络操作
struct http_message; // http_message 结构体，用于处理 HTTP 消息

// 解析 URL 的结构体，包含 Host、Port、Method 和 Params 字段，用于存储 URL
// 的解析结果
struct ParseUrlInfo {
  std::string Host;   // 主机名（域名或 IP）
  int Port;           // 端口号
  std::string Mothed; // HTTP 请求方法（如 GET、POST 等）
  std::string Path;   // URL路径
  std::string Params; // URL 中的参数部分
};

// MessageSystemHelp 类提供了一些辅助功能，用于处理消息系统中的不同任务
class MessageSystemHelp {
public:
  // 创建一个 Packet 数据包，传入消息 ID 和网络标识符
  static Packet *CreatePacket(Proto::MsgId msgId, NetIdentify *pIdentify);

  // 创建网络连接，指定网络类型、对象键、IP 地址和端口号
  static void CreateConnect(NetworkType iType, TagType tagType,
                            TagValue &tagValue, std::string ip, int port);

  // 分发消息包，根据消息 ID 和网络标识符进行处理
  static void DispatchPacket(const Proto::MsgId msgId, NetIdentify *pIdentify);
  static void DispatchPacket(const Proto::MsgId msgId,
                             google::protobuf::Message &proto,
                             NetIdentify *pIdentify);

  // 发送消息包，指定消息 ID 和消息内容
  static void SendPacket(const Proto::MsgId msgId, NetIdentify *pIdentify,
                         google::protobuf::Message &proto);
  static void SendPacket(const Proto::MsgId msgId,
                         google::protobuf::Message &proto, APP_TYPE appType,
                         int appId = 0);
  static void SendPacket(const Proto::MsgId msgId,
                         google::protobuf::Message &proto, TagKey *pTagKey,
                         APP_TYPE appType, int appId = 0);
  static void SendPacket(const Proto::MsgId msgId, TagKey *pTagKey,
                         APP_TYPE appType, int appId = 0);

  // 发送消息包，传入一个 Packet 对象
  static void SendPacket(Packet *pPacket);
  // 发送消息包，传入一个 Packet 对象，目标服务器类型，应用Id
  static void SendPacket(Packet *pPacket, APP_TYPE appType, int appId);

  // 向所有应用程序发送消息包
  static void SendPacketToAllApp(Proto::MsgId msgId,
                                 google::protobuf::Message &proto,
                                 APP_TYPE appType);

  // HTTP 相关功能

  // 发送 HTTP 请求，指定网络标识符、IP、端口、方法和参数
  static void SendHttpRequest(NetIdentify *pIdentify, std::string ip,
                              const int port, const std::string method,
                              const std::string path,
                              std::map<std::string, std::string> *pParams);

  // 发送 HTTP 响应，指定内容和大小
  static void SendHttpResponse(NetIdentify *pIdentify, const char *content,
                               int size);

  // 发送 404 错误响应
  static void SendHttpResponse404(NetIdentify *pIdentify);

  // 解析 HTTP 请求，返回 Packet
  // 对象，传入网络标识符、消息体、长度、是否是分块传输及 HTTP 消息结构
  static Packet *ParseHttp(NetIdentify *pIdentify, const char *s,
                           unsigned int bodyLen, const bool isChunked,
                           http_message *hm);

  // 解析 URL 地址，将结果保存到 ParseUrlInfo 结构体中
  static bool ParseUrl(const std::string &url, ParseUrlInfo &info);

  static std::string UrlEncode(const std::string &value);

protected:
  // 分发消息包，传入一个 Packet 对象
  static void DispatchPacket(Packet *packet);

  // HTTP 基础响应函数，发送自定义状态码的 HTTP 响应
  static void SendHttpResponseBase(NetIdentify *pIdentify, int status_code,
                                   const char *content, int size);
};
