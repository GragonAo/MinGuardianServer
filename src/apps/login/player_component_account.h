#pragma once

#include "libserver/common.h"  // 包含公共定义和类型的头文件
#include "libserver/component.h"  // 包含组件系统的头文件
#include "libserver/system.h"  // 包含系统接口定义的头文件

// PlayerComponentAccount 负责处理玩家账号相关的信息，如密码、选择的角色编号等
class PlayerComponentAccount : public Component<PlayerComponentAccount>,
                               public IAwakeFromPoolSystem<std::string> {  // 实现了从对象池中唤醒的系统接口

public:
  // 当对象从池中唤醒时调用，接受玩家的密码作为参数
  void Awake(std::string password) override;
  
  // 当对象返回池中时调用，重置成员变量以便重复使用
  void BackToPool() override;

  // 获取玩家密码
  std::string GetPassword() const;

  // 设置选定的玩家角色编号（sn 表示角色序列号）
  void SetSelectPlayerSn(uint64 sn);
  
  // 获取选定的玩家角色编号
  uint64 GetSelectPlayerSn() const;

  // 设置上次登录的游戏ID
  void SetLastGameId(int gameId);
  
  // 获取上次登录的游戏ID
  int GetLastGameId() const;

private:
  std::string _password;  // 存储玩家账号的密码
  uint64 _selectPlayerSn{0};  // 存储玩家选定的角色编号，默认初始化为 0
  int _lastGameId{0};  // 存储玩家上次登录的游戏 ID，默认初始化为 0
};
