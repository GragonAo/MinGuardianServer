#pragma once

// 抽象类 IWorld，提供基础的世界 ID 相关功能
class IWorld {
public:
  // 获取当前世界的 ID
  int GetWorldId() const;

protected:
  // 世界 ID，用于标识不同的世界实例，默认为 0
  int _worldId{0};
};
