#include "player_component_account.h"
#include "libserver/common.h"

// 当对象从池中唤醒时调用，初始化玩家的密码，并将选择的玩家角色编号和最后登录的游戏 ID 重置
void PlayerComponentAccount::Awake(const std::string password){
    _password = password;        // 设置玩家的密码
    _selectPlayerSn  = 0;        // 将选择的角色编号初始化为 0，表示未选择角色
    _lastGameId = INVALID_ID;    // 将最后登录的游戏 ID 设为无效 ID
}

// 当对象返回池中时调用，清空密码和重置角色编号及游戏 ID，以便该对象可以被重新使用
void PlayerComponentAccount::BackToPool(){
    _password = "";              // 清空密码
    _selectPlayerSn = 0;         // 重置选定的玩家角色编号
    _lastGameId = INVALID_ID;    // 重置最后登录的游戏 ID 为无效
}

// 返回玩家当前设置的密码
std::string PlayerComponentAccount::GetPassword() const{
    return _password;            // 返回存储的密码
}

// 设置选定的玩家角色编号
void PlayerComponentAccount::SetSelectPlayerSn(const uint64 sn){
    _selectPlayerSn = sn;        // 更新选定的角色序列号
}

// 返回选定的玩家角色编号
uint64 PlayerComponentAccount::GetSelectPlayerSn() const{
    return _selectPlayerSn;      // 返回当前选择的角色序列号
}

// 设置玩家最后登录的游戏 ID
void PlayerComponentAccount::SetLastGameId(int gameId){
    _lastGameId = gameId;        // 更新最后登录的游戏 ID
}

// 返回玩家最后登录的游戏 ID
int PlayerComponentAccount::GetLastGameId() const{
    return _lastGameId;          // 返回存储的最后登录的游戏 ID
}
