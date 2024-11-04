#pragma once

#include "libserver/protobuf/db.pb.h"

// PlayerComponent 是玩家的组件基类，定义了两个用于序列化和反序列化的虚函数
class PlayerComponent {
public:
  // 从传入的 Proto 对象中解析玩家数据，这个方法需要由具体的组件实现。
  // proto: 传入的玩家数据（Proto::Player 类型），从中提取组件所需要的数据。
  virtual void ParserFromProto(const Proto::Player &proto) = 0;

  // 将组件的数据序列化为 Proto 对象，这个方法需要由具体的组件实现。
  // pProto: 目标 Proto 对象（Proto::Player 类型），组件将其数据写入其中。
  virtual void SerializeToProto(Proto::Player *pProto) = 0;
};
