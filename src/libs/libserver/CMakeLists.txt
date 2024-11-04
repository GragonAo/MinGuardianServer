cmake_minimum_required(VERSION 2.8)  # 设置 CMake 的最低版本要求为 2.8

# 如果编译类型是 Release 模式
if (CMAKE_BUILD_TYPE STREQUAL Release) 
    set(MyProjectName server)  # 设置项目名称为 "server"
else()
    set(CMAKE_BUILD_TYPE Debug)  # 否则设置为 Debug 模式
    set(MyProjectName serverd)  # 设置项目名称为 "serverd" （Debug 版本名称）
endif()

# 输出一些调试信息到控制台
message(STATUS "CMAKE_BINARY_DIR:${CMAKE_BINARY_DIR}")  # 输出编译目录
message(STATUS "CMAKE_BUILD_TYPE:${CMAKE_BUILD_TYPE}")  # 输出编译类型
message(STATUS "ProjectName:${MyProjectName}")  # 输出项目名称

project(${MyProjectName})  # 设置项目名称

# 搜索当前目录、protobuf 目录和 mongoose 目录中的所有源文件，并将它们添加到 SRCS 变量中
aux_source_directory(. SRCS)
aux_source_directory(./protobuf SRCS)
aux_source_directory(./mongoose SRCS)

# 设置静态库的输出目录为 "../../../libs"
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "../../../libs")

# 设置 C++ 编译选项
set(CMAKE_CXX_FLAGS "-Wall -std=c++14 -pthread -lprotobuf -lyaml-cpp -DEPOLL")  # 设置标准为 C++14，启用多线程支持，链接 Protobuf 和 YAML-C++ 库，并启用 epoll
set(CMAKE_CXX_FLAGS_DEBUG "-D_DEBUG -O0 -g -ggdb")  # Debug 模式下的编译选项，启用调试信息并关闭优化
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")  # Release 模式下的编译选项，启用高级优化并禁用调试信息

# 将所有源文件编译成静态库
add_library(${MyProjectName} STATIC ${SRCS})
