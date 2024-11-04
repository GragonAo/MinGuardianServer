#pragma once

// Redis 相关键定义与超时时间配置

// 用户登录 Token 存储的 Redis 键前缀
// 该键用于存储用户的登录 Token，用于身份验证。
#define RedisKeyAccountTokey  "engine::token::"

// Token 过期时间（以秒为单位）
// 该时间定义了 Token 的有效期为 2 分钟（120秒），超时后将自动失效。
#define RedisKeyAccountTokeyTimeout  2 * 60  // 2分钟

// 用户角色登录状态存储的 Redis 键前缀
// 该键用于存储用户角色的登录信息，用于追踪角色的在线状态。
#define RedisKeyAccountOnlineLogin  "engine::online::login::"

// 用户角色登录状态的超时时间（以秒为单位）
// 该时间定义了用户登录状态的有效期为 6 分钟（360秒），超时后将自动失效。
#define RedisKeyAccountOnlineLoginTimeout  6 * 60  // 6分钟

// 用户角色游戏状态存储的 Redis 键前缀
// 该键用于存储用户角色的游戏状态，用于跟踪角色在游戏中的活动。
#define RedisKeyAccountOnlineGame  "engine::online::game::"

// 用户角色游戏状态的超时时间（以秒为单位）
// 该时间定义了游戏状态的有效期为 6 分钟（360秒），超时后将自动失效。
#define RedisKeyAccountOnlineGameTimeout  6 * 60  // 6分钟
