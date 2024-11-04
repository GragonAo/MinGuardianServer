#include "player_component_token.h"
#include "libserver/log4_help.h"

// 当组件被激活时，初始化 Token
void PlayerComponentToken::Awake(const std::string token)
{
    _token = token; // 将传入的 token 赋值给成员变量 _token
}

// 当组件被回收到对象池时，清空 Token
void PlayerComponentToken::BackToPool()
{
    _token = ""; // 重置 _token 为空字符串
}

// 检查传入的 token 是否与当前存储的 token 匹配
bool PlayerComponentToken::IsTokenValid(std::string token) const
{
    // 比较传入的 token 和当前存储的 token，确保长度一致并且内容相同
    return strncmp(_token.c_str(), token.c_str(), _token.length()) == 0;
}
