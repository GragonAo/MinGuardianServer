#pragma once

#include "mysql_base.h"
#include "libserver/singleton.h"

#include <functional>

class MysqlTableUpdate : public MysqlBase, public Singleton<MysqlTableUpdate>
{
public:
    // 构造函数：初始化类
    MysqlTableUpdate();
    
    // 析构函数：清理资源
    virtual ~MysqlTableUpdate();

    // 检查数据库的状态和版本，执行必要的更新
    void Check();

private:
    // 如果数据库不存在，则创建数据库
    bool CreateDatabaseIfNotExist();

    // 根据当前的数据库版本，更新到最新版本
    bool UpdateToVersion();
    
    // 执行特定版本的更新（这里是版本 00 的更新）
    bool Update00();

private:
    // 更新函数类型定义：每个更新函数返回一个布尔值，表示是否更新成功
    typedef std::function<bool(void)> OnUpdate;
    
    // 存储所有需要执行的更新函数
    std::vector<OnUpdate> _update_func;

    // 当前的数据库版本号
    int const _version = 0;
};
