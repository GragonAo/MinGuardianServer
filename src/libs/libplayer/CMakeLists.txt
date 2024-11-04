# 指定 CMake 的最低版本要求为 2.8。
cmake_minimum_required(VERSION 2.8)

# 根据构建类型来设置项目名称。
# 如果是 Release 版本，项目名称为 "player"；否则为 Debug 版本，项目名称为 "playerd"。
if (CMAKE_BUILD_TYPE STREQUAL Release) 
	set(MyProjectName player)  # Release 版本下的项目名称
else()
	set(CMAKE_BUILD_TYPE Debug)  # 如果未指定构建类型，则默认为 Debug
	set(MyProjectName playerd)   # Debug 版本下的项目名称
endif()

# 输出一些构建信息，用于调试或确认 CMake 配置是否正确。
message(STATUS "CMAKE_BINARY_DIR:${CMAKE_BINARY_DIR}")  # 输出二进制文件生成目录
message(STATUS "CMAKE_BUILD_TYPE:${CMAKE_BUILD_TYPE}")  # 输出当前构建类型（Debug 或 Release）
message(STATUS "ProjectName:${MyProjectName}")          # 输出项目名称

# 定义 CMake 项目名称
project(${MyProjectName})

# 将当前目录下的所有源文件添加到 SRCS 变量中
aux_source_directory(. SRCS)

# 将 ../libserver 目录下的所有源文件也添加到 SRCS 变量中
aux_source_directory(./../libserver SRCS)

# 设置包含目录
include_directories(../../libs)  # 包含路径为 ../../libs 目录，头文件从此处寻找

# 设置库文件的输出路径，静态库(.a)会被生成到 ../../../libs 目录中
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "../../../libs")

# 设置编译选项：
# -Wall: 显示所有警告信息
# -std=c++14: 使用 C++14 标准
# -pthread: 启用多线程支持
# -lprotobuf: 链接 Protocol Buffers 库
# -DEPOLL: 定义 EPOLL 宏（可能是用于事件驱动的 I/O 模型）
set(CMAKE_CXX_FLAGS "-Wall -std=c++14 -pthread -lprotobuf -DEPOLL")

# Debug 构建类型下的编译选项：
# -D_DEBUG: 定义 _DEBUG 宏，用于调试代码
# -O0: 不优化，保持调试信息的完整性
# -g, -ggdb: 生成调试信息，供调试器使用
set(CMAKE_CXX_FLAGS_DEBUG "-D_DEBUG -O0 -g -ggdb")

# Release 构建类型下的编译选项：
# -O3: 高级别的优化选项
# -DNDEBUG: 禁用断言 (assert)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# 最终生成静态库 (.a 文件)，库名称为 MyProjectName（即 player 或 playerd）
# SRCS 变量包含所有要编译的源文件
add_library(${MyProjectName} STATIC ${SRCS})
