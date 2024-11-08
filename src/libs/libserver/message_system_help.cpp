#include "message_system_help.h"
#include "component_help.h"
#include "entity_system.h"
#include "global.h"
#include "libserver/common.h"
#include "libserver/socket_object.h"
#include "log4_help.h"
#include "mongoose/mongoose.h"
#include "network_locator.h"
#include "object_pool_packet.h"
#include "thread_mgr.h"
#include <json/json.h>

// 函数: DispatchPacket
// 功能: 将消息包分发到线程管理器进行处理。
// 参数:
//   - pPacket: 指向需要分发的消息包指针。
void MessageSystemHelp::DispatchPacket(Packet *pPacket) {
  // 调用线程管理器的 DispatchPacket 方法，分发消息包
  ThreadMgr::GetInstance()->DispatchPacket(pPacket);
}

// 函数: DispatchPacket
// 功能: 根据消息 ID 创建消息包，并将其分发到线程管理器。
// 参数:
//   - msgId: 消息 ID。
//   - pIdentify: 指向网络标识符的指针，用于确定消息发送的目标。
void MessageSystemHelp::DispatchPacket(const Proto::MsgId msgId,
                                       NetIdentify *pIdentify) {
  // 创建消息包
  const auto pPacket = CreatePacket(msgId, pIdentify);
  // 分发消息包到线程管理器
  DispatchPacket(pPacket);
}

// 函数: DispatchPacket
// 功能: 创建一个包含 protobuf 数据的消息包，并分发给线程管理器处理。
// 参数:
//   - msgId: 消息 ID。
//   - proto: protobuf 消息，用于传输数据。
//   - pIdentify: 指向网络标识符的指针，标识消息的目标网络。
void MessageSystemHelp::DispatchPacket(const Proto::MsgId msgId,
                                       google::protobuf::Message &proto,
                                       NetIdentify *pIdentify) {
  // 创建消息包
  const auto pPacket = CreatePacket(msgId, pIdentify);
  // 将 protobuf 数据序列化到消息包缓冲区中
  pPacket->SerializeToBuffer(proto);
  // 将消息包分发到线程管理器
  DispatchPacket(pPacket);
}

// 函数: SendPacket
// 功能: 创建并发送包含 protobuf 数据的消息包，目标是指定的网络标识符。
// 参数:
//   - msgId: 消息 ID。
//   - pIdentify: 指向网络标识符的指针，用于标识发送目标。
//   - proto: protobuf 消息，表示需要发送的数据。
void MessageSystemHelp::SendPacket(const Proto::MsgId msgId,
                                   NetIdentify *pIdentify,
                                   google::protobuf::Message &proto) {
  // 创建消息包
  const auto pPacket = CreatePacket(msgId, pIdentify);
  // 将 protobuf 数据序列化到消息包缓冲区中
  pPacket->SerializeToBuffer(proto);
  // 发送消息包
  SendPacket(pPacket);
}

// 函数: SendPacket
// 功能: 根据应用类型和 ID，向目标应用发送包含 protobuf 数据的消息包。
// 参数:
//   - msgId: 消息 ID。
//   - proto: protobuf 消息，表示需要发送的数据。
//   - appType: 应用类型，标识消息的目标应用类型。
//   - appId: 应用 ID，标识消息的目标应用。
void MessageSystemHelp::SendPacket(const Proto::MsgId msgId,
                                   google::protobuf::Message &proto,
                                   APP_TYPE appType, int appId) {
  // 检查当前应用类型是否包含目标应用类型
  if ((Global::GetInstance()->GetCurAppType() & appType) != 0) {
    // 如果当前应用类型符合，直接分发消息
    DispatchPacket(msgId, proto, nullptr);
    return;
  }

  // 获取网络定位器，用于查找目标应用的网络标识符
  auto pNetworkLocator = ThreadMgr::GetInstance()
                             ->GetEntitySystem()
                             ->GetComponent<NetworkLocator>();
  // 根据应用类型和应用 ID 获取目标网络标识符
  auto networkIdentify = pNetworkLocator->GetNetworkIdentify(appType, appId);

  // 如果未找到有效的网络标识符，记录错误并返回
  if (networkIdentify.GetSocketKey()->NetType == NetworkType::None) {
    LOG_ERROR("can't find network. appType:" << GetAppName(appType)
                                             << " appId:" << appId);
    return;
  }

  // 创建消息包并序列化 protobuf 数据
  auto packet = CreatePacket(msgId, &networkIdentify);
  packet->SerializeToBuffer(proto);
  // 发送消息包
  SendPacket(packet);
}

void MessageSystemHelp::SendPacket(Packet *pPacket, APP_TYPE appType,
                                   int appId) {
  // 检查当前应用类型是否匹配
  if ((Global::GetInstance()->GetCurAppType() & appType) != 0) {
    DispatchPacket(pPacket); // 直接派发数据包
    return;
  }

  // 获取网络定位器组件
  auto pNetworkLocator = ThreadMgr::GetInstance()
                             ->GetEntitySystem()
                             ->GetComponent<NetworkLocator>();
  // 获取网络标识
  auto netIdentify = pNetworkLocator->GetNetworkIdentify(appType, appId);
  if (netIdentify.GetSocketKey()->NetType == NetworkType::None) {
    LOG_ERROR("can't find network. appType:" << GetAppName(appType)
                                             << " appId:" << appId);
    return; // 网络类型无效，输出错误日志并返回
  }

  // 设置数据包的socket信息
  const auto appSocketKey = netIdentify.GetSocketKey();
  pPacket->GetSocketKey()->Socket = appSocketKey->Socket;
  pPacket->GetSocketKey()->NetType = appSocketKey->NetType;
  SendPacket(pPacket); // 发送数据包
}

void MessageSystemHelp::SendPacket(const Proto::MsgId msgId,
                                   google::protobuf::Message &proto,
                                   TagKey *pTagKey, APP_TYPE appType,
                                   int appId) {
  // 检查当前应用类型是否匹配
  if ((Global::GetInstance()->GetCurAppType() & appType) != 0) {
    NetIdentify identify;
    identify.GetTagKey()->CopyFrom(pTagKey); // 复制标签信息
    DispatchPacket(msgId, proto, &identify); // 直接派发数据包
    return;
  }

  // 获取网络定位器组件
  auto pNetworkLocator = ThreadMgr::GetInstance()
                             ->GetEntitySystem()
                             ->GetComponent<NetworkLocator>();
  // 获取网络标识
  auto netIdentify = pNetworkLocator->GetNetworkIdentify(appType, appId);
  if (netIdentify.GetSocketKey()->NetType == NetworkType::None) {
    LOG_ERROR("can't find network. appType:" << GetAppName(appType)
                                             << " appId:" << appId);
    return; // 网络类型无效，输出错误日志并返回
  }

  // 设置网络标识
  NetIdentify identify;
  identify.GetSocketKey()->CopyFrom(netIdentify.GetSocketKey());
  identify.GetTagKey()->CopyFrom(pTagKey);

  // 创建数据包并序列化消息体
  auto packet = CreatePacket(msgId, &identify);
  packet->SerializeToBuffer(proto);
  SendPacket(packet); // 发送数据包
}

void MessageSystemHelp::SendPacket(const Proto::MsgId msgId, TagKey *pTagKey,
                                   APP_TYPE appType, int appId) {
  // 检查当前应用类型是否匹配
  if ((Global::GetInstance()->GetCurAppType() & appType) != 0) {
    NetIdentify identify;
    identify.GetTagKey()->CopyFrom(pTagKey); // 复制标签信息
    DispatchPacket(msgId, &identify);        // 直接派发数据包
    return;
  }

  // 获取网络定位器组件
  auto pNetworkLocator = ThreadMgr::GetInstance()
                             ->GetEntitySystem()
                             ->GetComponent<NetworkLocator>();
  // 获取网络标识
  auto netIdentify = pNetworkLocator->GetNetworkIdentify(appType, appId);
  if (netIdentify.GetSocketKey()->NetType == NetworkType::None) {
    LOG_ERROR("can't find network. appType:" << GetAppName(appType)
                                             << " appId:" << appId);
    return; // 网络类型无效，输出错误日志并返回
  }

  // 设置网络标识
  NetIdentify identify;
  identify.GetSocketKey()->CopyFrom(netIdentify.GetSocketKey());
  identify.GetTagKey()->CopyFrom(pTagKey);

  // 创建数据包并发送
  const auto packet = CreatePacket(msgId, &identify);
  SendPacket(packet); // 发送数据包
}

// 函数: SendPacketToAllApp
// 功能: 向所有符合条件的应用发送包含 protobuf 数据的消息包。
// 参数:
//   - msgId: 消息 ID。
//   - proto: protobuf 消息，表示需要发送的数据。
//   - appType: 应用类型，标识需要发送的目标应用。
void MessageSystemHelp::SendPacketToAllApp(Proto::MsgId msgId,
                                           google::protobuf::Message &proto,
                                           APP_TYPE appType) {
  // 检查当前应用类型是否符合目标应用类型
  if ((Global::GetInstance()->GetCurAppType() & appType) != 0) {
    // 如果符合，直接分发消息
    DispatchPacket(msgId, proto, nullptr);
  } else {
    // 获取网络定位器，用于查找目标应用的所有网络
    auto pNetworkLocator = ThreadMgr::GetInstance()
                               ->GetEntitySystem()
                               ->GetComponent<NetworkLocator>();
    // 获取目标应用类型对应的所有网络
    auto networks = pNetworkLocator->GetAppNetworks(appType);

    // 如果找到了匹配的网络
    if (!networks.empty()) {
      // 获取 TCP 连接器，如果不存在则获取监听器
      auto pNetworkLocator = ThreadMgr::GetInstance()
                                 ->GetEntitySystem()
                                 ->GetComponent<NetworkLocator>();
      auto networks = pNetworkLocator->GetAppNetworks(appType);
      // 遍历所有匹配的网络并发送消息包
      for (auto &one : networks) {
        Packet *pPacket = CreatePacket(msgId, &one);
        pPacket->SerializeToBuffer(proto);
        auto pConnector =
            pNetworkLocator->GetNetwork(one.GetSocketKey()->NetType);
        if (pConnector != nullptr)
          pConnector->SendPacket(pPacket);
      }

      return;
    }

    // 如果发送失败，记录警告日志
    LOG_WARN("failed to send packet. msgId:" << (int)msgId << " to appType:"
                                             << GetAppName(appType));
  }
}

// 函数: SendPacket
// 功能: 根据消息包中的网络标识符类型，将消息包发送到相应的网络或线程中。
// 参数:
//   - pPacket: 需要发送的消息包指针。
void MessageSystemHelp::SendPacket(Packet *pPacket) {
  // 检查消息包的 Socket 是否有效
  if (pPacket->GetSocketKey()->Socket == INVALID_SOCKET ||
      pPacket->GetSocketKey()->NetType == NetworkType::None) {
    // 如果 Socket 无效，直接将消息包分发到线程中
    DispatchPacket(pPacket);
    return;
  }

  // 获取网络定位器
  auto pNetworkLocator = ThreadMgr::GetInstance()
                             ->GetEntitySystem()
                             ->GetComponent<NetworkLocator>();
  const auto socketKey = pPacket->GetSocketKey();
  auto pNetwork = pNetworkLocator->GetNetwork(socketKey->NetType);

  if (pNetwork != nullptr) {
    pNetwork->SendPacket(pPacket);
    return;
  }

  // 如果找不到对应的网络，记录错误日志
  LOG_ERROR("failed to send packet." << dynamic_cast<NetIdentify *>(pPacket));
}

// 函数: CreatePacket
// 功能: 创建一个新的消息包，并从消息池中分配内存。
// 参数:
//   - msgId: 消息 ID。
//   - pIdentify: 指向网络标识符的指针，用于标识消息的目标网络。
// 返回值: 指向新创建的消息包指针。
Packet *MessageSystemHelp::CreatePacket(Proto::MsgId msgId,
                                        NetIdentify *pIdentify) {
  // 从动态消息包池中分配一个新的消息包
  return DynamicPacketPool::GetInstance()->MallocPacket(msgId, pIdentify);
}

void MessageSystemHelp::CreateConnect(NetworkType iType, TagType tagType,
                                      TagValue &tagValue, std::string ip,
                                      int port) {
  // 创建一个 Proto::NetworkConnect 对象，表示网络连接的参数
  Proto::NetworkConnect protoConn;

  // 设置连接的网络类型，将 NetworkType 转换为整数类型
  protoConn.set_network_type((int)iType);

  // 将 ObjectKey 对象序列化为 Proto 的 key 字段
  auto protoTag = protoConn.mutable_tag();
  protoTag->set_tag_type((Proto::TagType)tagType);
  auto protoValue = protoTag->mutable_tag_value();
  protoValue->set_value_int64(tagValue.KeyInt64);
  protoValue->set_value_str(tagValue.KeyStr.c_str());

  // 设置目标 IP 地址
  protoConn.set_ip(ip.c_str());

  // 设置目标端口号
  protoConn.set_port(port);

  // 调用 DispatchPacket 方法发送连接请求，消息类型为 MI_NetworkConnect
  DispatchPacket(Proto::MsgId::MI_NetworkConnect, protoConn, nullptr);
}

void MessageSystemHelp::SendHttpResponseBase(NetIdentify *pIdentify,
                                             int status_code,
                                             const char *content, int size) {
  // 获取网络定位器，用于查找 HTTP 监听对象
  auto pNetworkLocator = ThreadMgr::GetInstance()
                             ->GetEntitySystem()
                             ->GetComponent<NetworkLocator>();

  // 获取当前系统的 HTTP 监听网络
  auto pNetwork = pNetworkLocator->GetNetwork(NetworkType::HttpListen);

  // 如果找不到 HTTP 监听对象，记录错误并返回
  if (pNetwork == nullptr) {
    LOG_ERROR("can't find network. http send failed. socket:" << pIdentify);
    return;
  }

  // 创建一个 Packet 用于发送 HTTP 响应
  Packet *pPacket = CreatePacket(Proto::MsgId::MI_HttpInnerResponse, pIdentify);

  // 使用 stringstream 构造 HTTP 响应报文
  std::stringstream buffer;

  // 设置 HTTP 状态码和消息
  buffer << "HTTP/1.1 " << status_code << " " << mg_status_message(status_code)
         << "\r\n";

  // 关闭连接
  buffer << "Connection: close\r\n";

  // 设置响应的内容类型
  buffer << "Content-Type: application/json; charset=utf-8\r\n";

  // 设置内容的长度
  buffer << "Content-Length:" << size << "\r\n\r\n";

  // 如果有内容，写入内容
  if (size > 0) {
    buffer.write(content, size);
  }

  // 将 HTTP 响应报文序列化为 Packet 并准备发送
  pPacket->SerializeToBuffer(buffer.str().c_str(), buffer.tellp());

  // 如果启用了 HTTP 日志记录，输出 HTTP 请求
#if LOG_HTTP_OPEN
  LOG_HTTP("\r\n" << std::string(buffer.str().c_str(), buffer.tellp()).c_str());
#endif

  // std::cout << "Http Response:\n" << buffer.str() << std::endl;

  // 发送 Packet
  pNetwork->SendPacket(pPacket);
}

void MessageSystemHelp::SendHttpResponse404(NetIdentify *pIdentify) {
  // 调用 SendHttpResponseBase 发送 404 响应
  SendHttpResponseBase(pIdentify, 404, nullptr, 0);
}

void MessageSystemHelp::SendHttpResponse(NetIdentify *pIdentify,
                                         const char *content, int size) {
  // 调用 SendHttpResponseBase 发送 200 响应，带上内容和大小
  SendHttpResponseBase(pIdentify, 200, content, size);
}

bool MessageSystemHelp::ParseUrl(const std::string &url, ParseUrlInfo &info) {
  // mg_str 是 mongoose 库中的字符串结构，包含指向字符串的指针和长度
  struct mg_str uri;
  uri.p = url.c_str();    // 设置指向 URL 的指针
  uri.len = url.length(); // 设置 URL 长度

  unsigned int port = 0; // 存储解析出的端口
  struct mg_str scheme, user_info, host, path, query,
      fragment; // 分别表示 URL 各个部分

  // 使用 mongoose 提供的解析函数解析 URL，并将结果存储在相应变量中
  mg_parse_uri(uri, &scheme, &user_info, &host, &port, &path, &query,
               &fragment);

  // 将解析出的主机、端口、路径和查询参数存储在 info 对象中
  info.Host = std::string(host.p, host.len);
  info.Port = port == 0 ? 80 : port; // 如果没有指定端口，默认使用 80 端口
  info.Path = std::string(path.p, path.len);     // 路径部分
  info.Params = std::string(query.p, query.len); // 查询参数部分

  // std::cout << info.Host << " " << info.Port << " " << info.Mothed << " "
  //           << info.Params << std::endl;

  return true; // 返回 true 表示解析成功
}

Packet *MessageSystemHelp::ParseHttp(NetIdentify *pIdentify, const char *s,
                                     unsigned int bodyLen, const bool isChunked,
                                     http_message *hm) {
  // 创建 Proto::Http 对象，用于存储解析出的 HTTP 消息体
  Proto::Http proto;

  // 如果消息体长度大于 0，处理消息体内容
  if (bodyLen > 0) {
    if (isChunked) // 处理 chunked 编码的消息体
    {
      const auto end = s + bodyLen; // 指向消息体末尾
      mg_str tmp;
      while (true) {
        const int len = std::atoi(s); // 解析 chunk 的长度
        if (len == 0)
          break; // 当长度为 0 时，结束循环

        s = mg_skip(s, end, "\r\n", &tmp); // 跳过 chunk 长度部分
        s = mg_skip(s, end, "\r\n", &tmp); // 读取 chunk 数据
        proto.mutable_body()->append(tmp.p,
                                     tmp.len); // 将 chunk 数据追加到消息体中
      }
    } else // 处理非 chunked 的消息体
    {
      proto.mutable_body()->append(s,
                                   bodyLen); // 将消息体数据追加到 Proto 对象中
    }
  }

  // 设置默认的消息类型为 MI_HttpRequestBad，表示非法的 HTTP 请求
  Proto::MsgId msgId = Proto::MsgId::MI_HttpRequestBad;

  // 如果 HTTP 请求方法存在，处理不同的请求类型
  if (hm->method.len > 0) {
    // 处理 GET 请求
    do {
      if (mg_vcasecmp(&hm->method, "GET") == 0) // 比较请求方法是否为 GET
      {
        if (mg_vcasecmp(&hm->uri, "/login") ==
            0) // 如果 URI 为 /login，设置消息类型为 MI_HttpRequestLogin
        {
          msgId = Proto::MsgId::MI_HttpRequestLogin;
          break;
        }
      }

      // 如果不匹配任何已知的请求，记录错误日志
      LOG_ERROR("recv http method. "
                << " method:"
                << std::string(hm->method.p, hm->method.len).c_str()
                << " len:" << hm->method.len
                << " uri:" << std::string(hm->uri.p, hm->uri.len).c_str()
                << " len:" << hm->uri.len);
    } while (false);
  } else {
    // 如果没有请求方法，认为是 HTTP 响应，设置消息类型为 MI_HttpOuterResponse
    proto.set_status_code(hm->resp_code);
    msgId = Proto::MsgId::MI_HttpOuterResponse;
  }

  // 创建一个 Packet，用于传输解析后的 HTTP 请求/响应消息
  auto pPacket = CreatePacket(msgId, pIdentify);

  // 将 Proto::Http 对象序列化到 Packet 中
  pPacket->SerializeToBuffer(proto);
  return pPacket; // 返回构造的 Packet
}

void MessageSystemHelp::SendHttpRequest(
    NetIdentify *pIdentify, std::string ip, const int port,
    const std::string method, const std::string path,
    std::map<std::string, std::string> *pParams) {

  // 创建一个 Packet 用于发送 HTTP 请求
  Packet *pPacket = CreatePacket(Proto::MsgId::MI_HttpOuterRequest, pIdentify);

  // 使用 stringstream 构造 HTTP 请求报文
  std::stringstream buffer;

  // 检查请求方法是 GET 还是 POST
  if (method == "GET") {
    buffer << "GET " << path; // 设置请求路径

    // 如果有查询参数，构造查询字符串
    if (pParams != nullptr && !pParams->empty()) {
      buffer << "?";
      for (auto iter = pParams->begin(); iter != pParams->end(); ++iter) {
        buffer << UrlEncode(iter->first) << "=" << UrlEncode(iter->second)
               << "&"; // URL 编码参数
      }
      buffer.seekp(-1, std::ios_base::end); // 删除最后一个 '&'
    }
    buffer << " HTTP/1.1\r\n"; // 结束请求行
    // 设置通用头部
    buffer << "Host: " << ip.c_str() << ":" << port << "\r\n";
    buffer << "Content-Type: application/json; charset=utf-8\r\n"; // 设置内容类型为
                                                                   // JSON
    buffer << "Accept: */*, text/*, text/html\r\n";
    buffer << "\r\n"; // 结束头部

  } else if (method == "POST") {
    buffer << "POST " << path << " HTTP/1.1\r\n"; // 设置 POST 请求行
    // 设置 Host 头部
    buffer << "Host: " << ip.c_str() << ":" << port << "\r\n";
    buffer << "Accept: application/json\r\n"; // 设置接受的内容类型为 JSON
    buffer << "Connection: close\r\n";        // 关闭连接

    // 处理 POST 参数
    if (pParams != nullptr && !pParams->empty()) {
      // 创建 JSON 对象
      Json::Value jsonData;
      for (auto iter = pParams->begin(); iter != pParams->end(); ++iter) {
        jsonData[iter->first] = iter->second; // 将参数添加到 JSON 对象
      }

      // 将 JSON 对象转换为字符串
      Json::StreamWriterBuilder writer;
      std::string postData = Json::writeString(writer, jsonData);

      buffer << "Content-Type: application/json; charset=utf-8\r\n"; // 设置内容类型为
                                                                     // JSON
      buffer << "Content-Length: " << postData.size() << "\r\n"; // 设置内容长度
      buffer << "\r\n";                                          // 结束头部
      buffer << postData; // 写入 POST 数据

    } else {
      buffer << "Content-Length: 0\r\n"; // 没有参数时设置 Content-Length 为 0
      buffer << "\r\n";                  // 结束头部
    }
  }

  // 打印构建好的 HTTP 请求
  // std::cout << "SendHttpRequest (" << method << "): buffer:\n" <<
  // buffer.str() << std::endl;

  // 将 HTTP 请求报文序列化到 Packet 中
  pPacket->SerializeToBuffer(buffer.str().c_str(), buffer.tellp());

#if LOG_HTTP_OPEN
  // 如果启用了 HTTP 日志记录，输出 HTTP 请求
  auto msg = buffer.str();
  LOG_HTTP("\r\n" << std::string(msg.data(), buffer.tellp()).c_str());
#endif

  // 发送构造好的 Packet
  SendPacket(pPacket);
}

// URL 编码函数
std::string MessageSystemHelp::UrlEncode(const std::string &value) {
  std::ostringstream escaped;
  escaped.fill('0');
  escaped << std::hex;

  for (char c : value) {
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      escaped << c;
    } else {
      escaped << '%' << std::setw(2) << int((unsigned char)c);
    }
  }

  return escaped.str();
}
