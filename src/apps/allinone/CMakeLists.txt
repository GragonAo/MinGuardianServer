# 设置 CMake 最低版本要求为 2.8
cmake_minimum_required(VERSION 2.8)

# 判断当前构建类型是否为 Release，如果是则项目名设置为 allinone，否则设置为 allinoned
if (CMAKE_BUILD_TYPE STREQUAL Release) 
	set(MyProjectName allinone)
else()
	set(CMAKE_BUILD_TYPE Debug)  # 默认构建类型为 Debug
	set(MyProjectName allinoned)
endif()

# 打印当前的构建目录、构建类型和项目名称
message(STATUS "CMAKE_BINARY_DIR:${CMAKE_BINARY_DIR}")
message(STATUS "CMAKE_BUILD_TYPE:${CMAKE_BUILD_TYPE}")
message(STATUS "ProjectName:${MyProjectName}")

# 设置项目名称
project(${MyProjectName})

# 添加当前目录下的所有源文件到变量 SRCS 中
aux_source_directory(. SRCS)

# 添加 appmgr、login、dbmgr、game、space 目录下的源文件
# 并移除其中的 main.cpp 文件，以避免重复定义 main 函数
aux_source_directory(../appmgr SRCS)
list(REMOVE_ITEM SRCS "../appmgr/main.cpp")

aux_source_directory(../login SRCS)
list(REMOVE_ITEM SRCS "../login/main.cpp")

aux_source_directory(../dbmgr SRCS)
list(REMOVE_ITEM SRCS "../dbmgr/main.cpp")

aux_source_directory(../game SRCS)
list(REMOVE_ITEM SRCS "../game/main.cpp")

aux_source_directory(../space SRCS)
list(REMOVE_ITEM SRCS "../space/main.cpp")

# 添加 libserver、protobuf、mongoose、libplayer 等库的源文件
aux_source_directory(../../libs/libserver SRCS)
aux_source_directory(../../libs/libserver/protobuf SRCS)
aux_source_directory(../../libs/libserver/mongoose SRCS)
aux_source_directory(../../libs/libplayer SRCS)
aux_source_directory(../../libs/libresource SRCS)

# 包含库和应用程序的头文件目录
include_directories(../../libs)
include_directories(../../libs/libserver)
include_directories(../../libs/libserver/protobuf)
include_directories(../../libs/libserver/mongoose)

include_directories(../../apps)
include_directories(/usr/local/include)

# 设置可执行文件的输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../../bin")

# 设置编译选项，启用 C++14 标准，启用线程和相关库
set(CMAKE_CXX_FLAGS "-Wall -std=c++14 -pthread -lprotobuf -ljsoncpp -lyaml-cpp -llog4cplus -lhiredis -luuid -DEPOLL -Wno-unknown-pragmas")
# Debug 模式下的编译选项，启用调试信息
set(CMAKE_CXX_FLAGS_DEBUG "-D_DEBUG -O0 -g -ggdb")
# Release 模式下的编译选项，启用优化
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# 设置库搜索路径
link_directories(${CMAKE_BINARY_DIR}/../../../libs)
link_directories(${CMAKE_BINARY_DIR}/../../../../libs/linux)

# 添加可执行文件，源文件来自变量 SRCS
add_executable(${MyProjectName} ${SRCS})

# 链接所需的库（MySQL 库和其他常用库）
target_link_libraries(${MyProjectName} -L/usr/lib64/mysql -lmysqlclient -lpthread -lz -lm -lssl -lcrypto -ldl) # mysql

# 如果有需要，可以取消注释并链接自定义的 libserverd.a 库
#target_link_libraries(${MyProjectName} libserverd.a)
