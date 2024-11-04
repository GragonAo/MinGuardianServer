# 获取命令行参数
args=$1

# 设置默认的构建类型为 Debug
buildType="Debug"
# 设置默认不清理构建文件
clean=false

# 检查传入参数是否为 "release"，如果是则将构建类型设置为 Release
if [ ${args}v = "release"v ];then
    buildType="Release"
# 检查传入参数是否为 "clean"，如果是则设置 clean 为 true
elif [ ${args}v = "clean"v ];then
    clean=true
fi

# 定义一个函数来执行构建或清理操作
build(){
    # 遍历当前目录下的所有文件夹
    for i in `ls -d */`;do 
        # 进入每个文件夹
        cd $i
        
        # 如果参数是 clean，则执行清理操作
        if ${clean};then            
            make clean  # 使用 make clean 清理构建文件
            rm -f CMakeCache.txt  # 删除 CMake 缓存文件
            rm -rf CMakeFiles  # 删除 CMakeFiles 文件夹
            rm -rf Debug  # 删除 Debug 文件夹（如果存在）
            rm -f cmake_install.cmake  # 删除 CMake 安装文件
            rm -f Makefile  # 删除 Makefile 文件
        else
            # 否则执行构建操作，传递构建类型参数
            cmake3 -DCMAKE_BUILD_TYPE=${buildType} ./ 
            make -j4  # 使用 4 个线程并行构建
        fi
        # 返回到上一级目录
        cd ..
    done
}

# 注释掉的部分：可能是之前用来构建 src/libs/ 目录中的项目
# cd src/libs/
# build

# 切换到 src/apps/ 目录并执行构建或清理操作
cd src/apps/
build

# 切换到 ../tools/ 目录并执行构建或清理操作
cd ../tools/
build
