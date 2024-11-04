#pragma once

#include "libserver/protobuf/db.pb.h"
#include <ostream>

// 定义三维向量类，用于表示空间中的点或向量
struct Vector3 {
  // 构造函数，初始化向量的 X、Y、Z 坐标
  Vector3(const float x, const float y, const float z) : X(x), Y(y), Z(z) {}

  // 从 Protobuf 的 Vector3 消息中解析位置数据
  void ParserFromProto(Proto::Vector3 position);
  
  // 将当前 Vector3 实例序列化为 Protobuf 的 Vector3 消息
  void SerializeToProto(Proto::Vector3 *pProto) const;
  
  // 计算当前向量与另一个 Vector3 的欧几里得距离
  double GetDistance(Vector3 point) const;

  // 向量的 X、Y、Z 坐标值，默认为 0
  float X{0};
  float Y{0};
  float Z{0};

  // 静态成员，表示零向量 (0, 0, 0)
  static Vector3 Zero;
};

// 重载流插入运算符，方便将 Vector3 输出到标准输出流
std::ostream &operator<<(std::ostream &os, Vector3 &v);
