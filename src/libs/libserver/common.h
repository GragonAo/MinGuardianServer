#pragma once

#include <climits>
#include "protobuf/proto_id.pb.h"
#include "protobuf/db.pb.h"
#include "protobuf/msg.pb.h"

#ifndef PATH_MAX  
// 定义路径的最大长度为 1024，如果系统未定义 PATH_MAX
#define PATH_MAX 1024  
#endif  

// 定义无效 ID 为 -1
#define INVALID_ID (-1)

// 定义不同平台的宏，用于判断当前是 Windows 还是 Unix 平台
#define PLATFORM_WIN32 0
#define PLATFORM_UNIX 1

// 根据编译环境判断当前平台，设置 ENGINE_PLATFORM 宏
#if defined( __WIN32__ ) || defined( WIN32 ) || defined( _WIN32 )
// 如果是 Windows 平台，定义 ENGINE_PLATFORM 为 PLATFORM_WIN32
#  define ENGINE_PLATFORM PLATFORM_WIN32
#else
// 否则定义为 Unix 平台
#  define ENGINE_PLATFORM PLATFORM_UNIX
#endif

// 根据不同平台定义字符串和文件操作相关的函数
#if ENGINE_PLATFORM != PLATFORM_WIN32
#include <string.h>
#include <stdint.h>

// Unix 平台使用 strcasecmp 比较字符串，access 检查文件访问权限
#define engine_stricmp strcasecmp
#define engine_access access
// strncpy 用于拷贝字符串，定义适配不同平台的函数
#define engine_strncpy(dest, destsz, src, srcsz) strncpy(dest, src, srcsz)

#else
// Windows 平台使用 _stricmp 比较字符串，_access 检查文件访问权限
#define engine_stricmp _stricmp
#define engine_access _access
// strncpy_s 是 Windows 上的安全字符串拷贝函数
#define engine_strncpy(dest, destsz, src, srcsz) strncpy_s(dest, destsz, src, srcsz)

#endif

// 在 Unix 平台下，定义 SOCKET 和 INVALID_SOCKET
#if ENGINE_PLATFORM != PLATFORM_WIN32

// Unix 平台下 SOCKET 定义为 int 类型，INVALID_SOCKET 为 -1
#define SOCKET int
#define INVALID_SOCKET -1

#include <stdint.h>
// 定义 uint64 和 uint32 类型，用于跨平台统一类型
typedef uint64_t	uint64;
typedef uint32_t	uint32;

#else

// Windows 平台下，定义 FD_SETSIZE 为 1024，用于控制文件描述符集的大小
#define FD_SETSIZE 1024

#include <Ws2tcpip.h>

// 在 Windows 平台下定义 uint64 和 uint32 类型
typedef unsigned __int64	uint64;
typedef unsigned __int32	uint32;

#endif 

// 绑定函数的宏定义，用于简化 std::bind 调用
// BindFunP4 绑定有 4 个参数的成员函数
#define BindFunP4(_self, _f) std::bind ( _f, _self, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4 )
// BindFunP3 绑定有 3 个参数的成员函数
#define BindFunP3(_self, _f) std::bind ( _f, _self, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 )
// BindFunP2 绑定有 2 个参数的成员函数
#define BindFunP2(_self, _f) std::bind ( _f, _self, std::placeholders::_1, std::placeholders::_2 )
// BindFunP1 绑定有 1 个参数的成员函数
#define BindFunP1(_self, _f) std::bind ( _f, _self, std::placeholders::_1 )
// BindFunP0 绑定无参数的成员函数
#define BindFunP0(_self, _f) std::bind ( _f, _self)
