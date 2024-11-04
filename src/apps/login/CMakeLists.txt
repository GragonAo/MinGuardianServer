# 指定CMake的最低版本要求为2.8
cmake_minimum_required(VERSION 2.8)

# 如果编译类型是Release，项目名称为login，否则是logind
if (CMAKE_BUILD_TYPE STREQUAL Release)
    set(MyProjectName login)
else()
    # 默认设置为Debug类型
    set(CMAKE_BUILD_TYPE Debug)
    set(MyProjectName logind)
endif()

# 打印一些信息，帮助调试或查看CMake的构建配置
message(STATUS "CMAKE_BINARY_DIR:${CMAKE_BINARY_DIR}")  # 打印二进制输出目录
message(STATUS "CMAKE_BUILD_TYPE:${CMAKE_BUILD_TYPE}")  # 打印当前构建类型
message(STATUS "ProjectName:${MyProjectName}")          # 打印项目名称

# 定义项目名称
project(${MyProjectName})

# 指定要编译的源代码目录
aux_source_directory(. SRCS)                             # 当前目录下的源文件
aux_source_directory(../../libs/libserver SRCS)          # libserver目录下的源文件
aux_source_directory(../../libs/libserver/protobuf SRCS) # protobuf目录下的源文件
aux_source_directory(../../libs/libserver/mongoose SRCS) # mongoose目录下的源文件
aux_source_directory(../../libs/libplayer SRCS)          # libplayer目录下的源文件
aux_source_directory(../../libs/libresource SRCS)

# 指定头文件的搜索路径
include_directories(../../libs)                          # 包含libs目录
include_directories(/usr/local/include)                  # 包含系统的/usr/local/include目录

# 设置编译输出的二进制文件目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../../bin")

# 设置编译器标志，用于启用各种编译选项
# -Wall: 启用所有警告
# -std=c++14: 使用C++14标准
# -pthread: 启用多线程支持
# -lprotobuf: 链接protobuf库
# -ljsoncpp: 链接jsoncpp库
# -lyaml-cpp: 链接yaml-cpp库
# -llog4cplus: 链接log4cplus库
# -DEPOLL: 定义EPOLL宏
set(CMAKE_CXX_FLAGS "-Wall -std=c++14 -pthread -lprotobuf -lyaml-cpp -llog4cplus -ljsoncpp -lhiredis -luuid -DEPOLL -Wno-unknown-pragmas")

# 设置Debug模式的编译器标志
# -D_DEBUG: 定义_DEBUG宏，用于调试
# -O0: 不优化代码
# -g -ggdb: 生成调试信息
set(CMAKE_CXX_FLAGS_DEBUG "-D_DEBUG -O0 -g -ggdb")

# 设置Release模式的编译器标志
# -O3: 最高优化等级
# -DNDEBUG: 禁用断言和调试相关的代码
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# 指定库文件的链接路径
link_directories(${CMAKE_BINARY_DIR}/../../../libs)      # 链接本地libs目录中的库
link_directories(${CMAKE_BINARY_DIR}/../../../../libs/linux) # 链接Linux平台的库目录

# 生成可执行文件
add_executable(${MyProjectName} ${SRCS})

# 以前注释掉的代码，可以根据需要启用，链接libserverd库
# target_link_libraries(${MyProjectName} libserverd.a)
