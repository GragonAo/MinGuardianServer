# 指定 CMake 的最低版本要求
cmake_minimum_required(VERSION 2.8)

# 如果构建类型是 Release，则将项目名称设置为 appmgr
if (CMAKE_BUILD_TYPE STREQUAL Release) 
	set(MyProjectName appmgr)
else()
    # 否则，默认构建类型为 Debug，项目名称设置为 appmgrd（d 表示 Debug 版本）
	set(CMAKE_BUILD_TYPE Debug)
	set(MyProjectName appmgrd)
endif()

# 打印一些构建信息供调试使用
message(STATUS "CMAKE_BINARY_DIR:${CMAKE_BINARY_DIR}")
message(STATUS "CMAKE_BUILD_TYPE:${CMAKE_BUILD_TYPE}")
message(STATUS "ProjectName:${MyProjectName}")

# 定义项目名称
project(${MyProjectName})

# 搜索当前目录下和指定路径下的所有源文件，并将其存储到 SRCS 变量中
aux_source_directory(. SRCS)
aux_source_directory(../../libs/libserver SRCS)
aux_source_directory(../../libs/libserver/protobuf SRCS)
aux_source_directory(../../libs/libserver/mongoose SRCS)
aux_source_directory(../../libs/libresource SRCS)

# 包含目录，以便项目中可以引用这些路径下的头文件
include_directories(../../libs)  # 包含项目库目录
include_directories(../../../../include/common)  # 包含 common 目录
include_directories(/usr/local/include)  # 包含系统头文件目录

# 设置可执行文件的输出目录为 ../../../bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../../bin")

# 设置编译选项：启用 Wall 警告，使用 C++14 标准，启用多线程，并链接一些库（protobuf、jsoncpp、yaml-cpp、log4cplus）
set(CMAKE_CXX_FLAGS "-Wall -std=c++14 -pthread -lprotobuf -lyaml-cpp -llog4cplus -ljsoncpp -lhiredis -luuid -DEPOLL -Wno-unknown-pragmas")

# 设置 Debug 模式下的编译选项：启用调试符号和优化选项
set(CMAKE_CXX_FLAGS_DEBUG "-D_DEBUG -O0 -g -ggdb")

# 设置 Release 模式下的编译选项：启用优化和禁用调试符号
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# 设置链接库的目录
link_directories(${CMAKE_BINARY_DIR}/../../../libs)  # 链接项目库的目录
link_directories(${CMAKE_BINARY_DIR}/../../../../libs/linux)  # 链接系统库的目录

# 定义要生成的可执行文件并链接源文件
add_executable(${MyProjectName} ${SRCS})

# 如果需要，指定链接静态库 libserverd.a，当前被注释掉
# target_link_libraries(${MyProjectName} libserverd.a)
