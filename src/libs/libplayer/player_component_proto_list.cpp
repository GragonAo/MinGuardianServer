#include "libserver/protobuf/msg.pb.h"
#include "player_component_proto_list.h"
#include <sstream>

// BackToPool 函数用于在该组件被放回对象池时调用，清理所有玩家数据并释放内存
void PlayerComponentProtoList::BackToPool() {
  // 遍历存储的玩家 Proto 数据的 map，并释放每个玩家的 std::stringstream 对象
  for (auto iter = _protos.begin(); iter != _protos.end(); ++iter) {
    delete iter->second;  // 释放 std::stringstream* 对象，防止内存泄漏
  }
  _protos.clear();  // 清空 map，移除所有已存储的数据
}

// Parse 函数用于将 Proto::PlayerList 中的玩家数据解析并序列化为 std::stringstream
// proto: 传入的 Proto::PlayerList 对象，包含所有玩家数据
void PlayerComponentProtoList::Parse(Proto::PlayerList &proto) {
  // 获取玩家列表的大小
  const int size = proto.player_size();
  for (int i = 0; i < size; i++) {
    // 获取每个玩家的唯一标识符 (sn)
    auto playerSn = proto.player(i).sn();
    // 创建一个新的 stringstream 对象用于保存玩家的序列化数据
    std::stringstream *pStream = new std::stringstream();
    // 将该玩家的 Proto 数据序列化为输出流
    proto.player(i).SerializeToOstream(pStream);
    // 将该玩家的 sn 与序列化流存储到 map 中
    _protos[playerSn] = pStream;
  }
}

// GetProto 函数根据玩家的唯一标识符 (sn) 获取对应的玩家 Proto 数据流
// playerSn: 玩家唯一标识符 (uint64)
// 返回值: 指向 std::stringstream* 对象的指针，包含该玩家的 Proto 数据，如果找不到则返回 nullptr
std::stringstream *PlayerComponentProtoList::GetProto(const uint64 playerSn) {
  // 在 map 中查找指定 sn 对应的玩家数据流
  const auto iter = _protos.find(playerSn);
  // 如果找不到对应的玩家数据，返回 nullptr
  if (iter == _protos.end()) {
    return nullptr;
  }
  // 返回找到的 std::stringstream* 对象
  return iter->second;
}
