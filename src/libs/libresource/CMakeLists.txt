cmake_minimum_required(VERSION 2.8)  # 设置 CMake 的最低版本要求

# 根据构建类型设置项目名称
if (CMAKE_BUILD_TYPE STREQUAL Release) 
    set(MyProjectName resource)  # 释放模式下项目名称
else()
    set(CMAKE_BUILD_TYPE Debug)   # 默认使用 Debug 模式
    set(MyProjectName resourced)   # 调试模式下项目名称
endif()

# 输出当前的 CMake 二进制目录、构建类型和项目名称，方便调试
message(STATUS "CMAKE_BINARY_DIR:${CMAKE_BINARY_DIR}")
message(STATUS "CMAKE_BUILD_TYPE:${CMAKE_BUILD_TYPE}")
message(STATUS "ProjectName:${MyProjectName}")

project(${MyProjectName})  # 定义项目名称

# 收集当前目录和 libserver 目录下的所有源文件
aux_source_directory(. SRCS)                     # 当前目录
aux_source_directory(./../libserver SRCS)       # libserver 目录

# 设置静态库的输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "../../../libs")

# 设置编译选项
set(CMAKE_CXX_FLAGS "-Wall -std=c++14 -pthread -lprotobuf -DEPOLL")  # 基本编译选项
set(CMAKE_CXX_FLAGS_DEBUG "-D_DEBUG -O0 -g -ggdb")                   # 调试模式编译选项
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")                            # 释放模式编译选项

# 创建静态库
add_library(${MyProjectName} STATIC ${SRCS})  # 使用收集的源文件创建静态库
