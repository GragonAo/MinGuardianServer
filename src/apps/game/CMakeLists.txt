# 设置 CMake 的最低版本要求为 2.8
cmake_minimum_required(VERSION 2.8)

# 根据构建模式选择项目名称：Release 模式为 game，Debug 模式为 gamed
if (CMAKE_BUILD_TYPE STREQUAL Release) 
    set(MyProjectName game)  # Release 模式时项目名为 game
else()
    set(CMAKE_BUILD_TYPE Debug)  # 默认设置为 Debug 模式
    set(MyProjectName gamed)  # Debug 模式时项目名为 gamed
endif()

# 打印一些调试信息：二进制目录、构建模式、项目名称
message(STATUS "CMAKE_BINARY_DIR:${CMAKE_BINARY_DIR}")
message(STATUS "CMAKE_BUILD_TYPE:${CMAKE_BUILD_TYPE}")
message(STATUS "ProjectName:${MyProjectName}")

# 设置项目名称
project(${MyProjectName})

# 指定源文件目录并将所有源文件添加到 SRCS 变量中
aux_source_directory(. SRCS)  # 当前目录的源文件
aux_source_directory(../../libs/libserver SRCS)  # libserver 库目录中的源文件
aux_source_directory(../../libs/libserver/protobuf SRCS)  # libserver 的 protobuf 源文件
aux_source_directory(../../libs/libserver/mongoose SRCS)  # libserver 的 mongoose 源文件
aux_source_directory(../../libs/libplayer SRCS)  # libplayer 库目录中的源文件
aux_source_directory(../../libs/libresource SRCS)

# 包含头文件目录
include_directories(../../libs)  # 包含 libs 目录
include_directories(/usr/local/include)  # 包含系统路径下的头文件

# 设置编译后可执行文件的输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../../bin")

# 设置编译标志：
# -Wall: 打开所有警告
# -std=c++14: 使用 C++14 标准
# -pthread: 支持多线程
# -lprotobuf, -lyaml-cpp, -llog4cplus: 链接 protobuf、yaml-cpp、log4cplus 库
# -DEPOLL: 定义 EPOLL 宏
# -Wno-unknown-pragmas: 忽略未知的预处理指令警告
set(CMAKE_CXX_FLAGS "-Wall -std=c++14 -pthread -lprotobuf -lyaml-cpp -llog4cplus -ljsoncpp -lhiredis -luuid -DEPOLL -Wno-unknown-pragmas")

# 为 Debug 模式设置额外的编译选项
set(CMAKE_CXX_FLAGS_DEBUG "-D_DEBUG -O0 -g -ggdb")  # 开启调试信息，不做优化

# 为 Release 模式设置优化选项
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")  # 启用最高级别优化，并关闭调试信息

# 设置链接库的搜索目录
link_directories(${CMAKE_BINARY_DIR}/../../../libs)  # 本地项目的库目录
link_directories(${CMAKE_BINARY_DIR}/../../../../libs/linux)  # Linux 系统上的库目录

# 创建可执行文件
add_executable(${MyProjectName} ${SRCS})  # 将 SRCS 中的源文件编译为可执行文件

# 可以选择链接静态库，取消注释以启用
# target_link_libraries(${MyProjectName} libserverd.a)
