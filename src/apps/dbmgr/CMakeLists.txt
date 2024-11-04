cmake_minimum_required(VERSION 2.8)  # 设置 CMake 的最低版本要求为 2.8

# 根据编译类型设置项目名称
if (CMAKE_BUILD_TYPE STREQUAL Release) 
    set(MyProjectName dbmgr)  # Release 模式项目名为 dbmgr
else()
    set(CMAKE_BUILD_TYPE Debug)  # Debug 模式下设置为 Debug 类型
    set(MyProjectName dbmgrd)  # Debug 模式项目名为 dbmgrd
endif()

# 输出一些调试信息到控制台
message(STATUS "CMAKE_BINARY_DIR:${CMAKE_BINARY_DIR}")  # 输出编译目录
message(STATUS "CMAKE_BUILD_TYPE:${CMAKE_BUILD_TYPE}")  # 输出编译类型
message(STATUS "ProjectName:${MyProjectName}")  # 输出项目名称

project(${MyProjectName})  # 定义项目

# 搜索当前目录和其他库目录中的所有源文件，并将它们添加到 SRCS 变量中
aux_source_directory(. SRCS)
aux_source_directory(../../libs/libserver SRCS)
aux_source_directory(../../libs/libserver/protobuf SRCS)
aux_source_directory(../../libs/libserver/mongoose SRCS)

# 设置包含目录，用于包含头文件
include_directories(../../libs)  # 添加 libs 目录
include_directories(../../../../include/common)  # 添加 common 头文件目录
include_directories(/usr/local/include)  # 添加系统的 /usr/local/include 目录

# 设置可执行文件的输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../../bin")

# 设置编译选项
set(CMAKE_CXX_FLAGS "-Wall -std=c++14 -pthread -lprotobuf -lyaml-cpp -llog4cplus -ljsoncpp -lhiredis -luuid -DEPOLL -Wno-unknown-pragmas") # 编译选项，启用 C++14 标准、线程支持、protobuf、yaml-cpp 等库的链接
set(CMAKE_CXX_FLAGS_DEBUG "-D_DEBUG -O0 -g -ggdb")  # Debug 模式下启用调试信息
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")  # Release 模式下启用优化并禁用调试信息

# 设置链接库目录
link_directories(${CMAKE_BINARY_DIR}/../../../libs)  # 链接项目的库文件目录
link_directories(${CMAKE_BINARY_DIR}/../../../../libs/linux)  # 链接外部库目录

# 添加可执行文件
add_executable(${MyProjectName} ${SRCS})

# 链接 MySQL 客户端库和其他必要的库
target_link_libraries(${MyProjectName} -L/usr/lib64/mysql -lmysqlclient -lpthread -lz -lm -lssl -lcrypto -ldl)

# 手动链接静态库 libserverd.a
# target_link_libraries(${MyProjectName} libserverd.a)
