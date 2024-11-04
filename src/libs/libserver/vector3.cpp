#include "vector3.h"
#include "libserver/protobuf/db.pb.h"
#include <cmath>

// 初始化静态成员变量 Vector3::Zero 为 (0, 0, 0)
Vector3 Vector3::Zero(0, 0, 0);

// 从 Protobuf 的 Vector3 消息中解析位置数据
void Vector3::ParserFromProto(Proto::Vector3 position) {
    X = position.x();  // 从 Protobuf 中获取 X 坐标
    Y = position.y();  // 从 Protobuf 中获取 Y 坐标
    Z = position.z();  // 从 Protobuf 中获取 Z 坐标
}

// 将当前 Vector3 实例序列化为 Protobuf 的 Vector3 消息
void Vector3::SerializeToProto(Proto::Vector3 *pProto) const {
    pProto->set_x(X);  // 设置 X 坐标
    pProto->set_y(Y);  // 设置 Y 坐标
    pProto->set_z(Z);  // 设置 Z 坐标
}

// 计算当前点到另一个 Vector3 点的欧几里得距离
double Vector3::GetDistance(Vector3 point) const {
    float dx = X - point.X;  // 计算 X 坐标的差值
    float dy = Y - point.Y;  // 计算 Y 坐标的差值
    float dz = Z - point.Z;  // 计算 Z 坐标的差值
    return sqrt(dx * dx + dy * dy + dz * dz);  // 使用勾股定理计算距离
}

// 重载流插入操作符，将 Vector3 的内容以 (X, Y, Z) 的格式输出
std::ostream &operator<<(std::ostream &os, Vector3 &v) {
    os << "(" << v.X << ", " << v.Y << ", " << v.Z << ")";
    return os;
}
