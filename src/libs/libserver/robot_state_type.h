#pragma once

// 定义机器人状态类型的枚举
enum class RobotStateType
{
    None = -1,               // 无效状态

    Http_Connecting = 0,     // 正在连接 HTTP
    Http_Connected,		    // HTTP 连接成功

    Login_Connecting,	    // 正在连接 Login 服务器
    Login_Connected,	        // Login 服务器连接成功
    Login_Logined,            // 登录成功

    // Login_CreatePlayer,     // 创建角色
    Login_SelectPlayer,       // 选择角色

    Game_Connecting,	        // 正在连接游戏服务器
    Game_Connected,	        // 游戏服务器连接成功
    Game_Logined,             // 游戏服务器登录成功

    Space_EnterWorld,         // 进入游戏世界

    End,                      // 状态结束标识
};

// 定义详细的状态描述信息
const char RobotStateTypeName[][255] = {
    "[Http ] Connecting  :",   // HTTP 连接中
    "[Http ] Connected   :",   // HTTP 已连接

    "[Login] Connecting  :",   // 登录服务器连接中
    "[Login] Connected   :",   // 登录服务器已连接
    "[Login] Logined     :",   // 登录成功

    //"[Login] CreatePlayer:", // 创建角色（已注释）
    "[Login] SelectPlayer:",   // 选择角色

    "[Game ] Connecting  :",   // 游戏服务器连接中
    "[Game ] Connected   :",   // 游戏服务器已连接
    "[Game ] Logined     :",   // 游戏服务器登录成功

    "[Space] EnterWorld  :",   // 进入游戏世界
};

// 定义简短的状态描述信息
const char RobotStateTypeShortName[][255] = {
    "Http-Connecting",         // HTTP 连接中
    "Http-Connected",          // HTTP 已连接

    "Login-Connecting",        // 登录服务器连接中
    "Login-Connected",         // 登录服务器已连接
    "Login-Logined",           // 登录成功
    //"Login-CreatePlayer",     // 创建角色（已注释）
    "Login-SelectPlayer",      // 选择角色

    "Game-Connecting",         // 游戏服务器连接中
    "Game-Connected",          // 游戏服务器已连接
    "Game-Logined",            // 游戏服务器登录成功

    "Space-EnterWorld",        // 进入游戏世界
};

// 获取简短状态名称
inline const char* GetRobotStateTypeShortName(RobotStateType stateType)
{
    // 检查状态是否有效
    if (stateType <= RobotStateType::None || stateType >= RobotStateType::End)
    {
        return "unknown"; // 如果状态无效，返回 "unknown"
    }

    // 返回对应的简短状态名称
    return RobotStateTypeShortName[(int)stateType];
}

// 获取详细状态名称
inline const char* GetRobotStateTypeName(RobotStateType stateType)
{
    // 检查状态是否有效
    if (stateType <= RobotStateType::None || stateType >= RobotStateType::End)
    {
        return "unknown"; // 如果状态无效，返回 "unknown"
    }

    // 返回对应的详细状态名称
    return RobotStateTypeName[(int)stateType];
}
