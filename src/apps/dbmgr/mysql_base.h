#pragma once

#include "libserver/common.h"  // 公共库文件
#include "libserver/yaml.h"    // YAML 配置文件解析库

#include <mysql/mysql.h>  // MySQL C API 头文件

class MysqlBase
{
public:
    // 初始化 MySQL 连接，返回 true 表示成功，false 表示失败
    bool ConnectInit();
    
    // 虚函数，用于关闭数据库连接，子类可重写
    virtual void Disconnect();

    // 检查 MySQL 错误并返回错误代码
    int CheckMysqlError() const;

    // 执行 SQL 查询，返回执行结果并通过 affected_rows 输出受影响的行数
    bool Query(const char* sql, my_ulonglong& affected_rows);
    
    // 从查询结果中获取一行数据
    MYSQL_ROW Fetch() const;

    // 静态方法，用于从 MYSQL_ROW 数据中获取各类字段值
    static int GetInt(MYSQL_ROW row, int index);
    static unsigned int GetUint(MYSQL_ROW row, int index);
    static uint64 GetUint64(MYSQL_ROW row, int index);
    static char* GetString(MYSQL_ROW row, int index);

    // 获取 BLOB 数据，读取到缓冲区中
    int GetBlob(MYSQL_ROW row, int index, char* buf, unsigned long size) const;
    
    // 获取 BLOB 数据，并将其存储为字符串形式
    void GetBlob(MYSQL_ROW row, int index, std::string& protoStr) const;

protected:
    // 数据库配置指针，指向一个 DBConfig 对象，用于管理数据库连接信息
    DBConfig* _pDbCfg{ nullptr };

    // MySQL 连接相关变量
    MYSQL* _pMysql{ nullptr };         // MYSQL 连接对象
    MYSQL_RES* _pMysqlRes{ nullptr };  // 查询结果集
    int _numFields{ 0 };               // 字段数量
    MYSQL_FIELD* _pMysqlFields{ nullptr };  // 字段描述信息
};
