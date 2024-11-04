#include "mysql_base.h"
#include "libserver/log4_help.h"

bool MysqlBase::ConnectInit()
{
    // 断开现有的连接（如果有），以确保连接干净
    Disconnect();

    // 初始化MySQL连接句柄
    _pMysql = mysql_init(nullptr);
    if (_pMysql == nullptr)
    {
        // 检查初始化错误，并记录日志
        CheckMysqlError();
        return false;
    }

    // 设置MySQL连接超时时间为10秒
    int outtime = 10;
    mysql_options(_pMysql, MYSQL_OPT_CONNECT_TIMEOUT, &outtime);

    // 禁用自动重新连接功能（断线不自动重连）
    bool reConnect = false;
    mysql_options(_pMysql, MYSQL_OPT_RECONNECT, &reConnect);

    // 初始化字段计数器和字段指针
    _numFields = 0;
    _pMysqlFields = nullptr;
    _pMysqlRes = nullptr;

    return true;
}

void MysqlBase::Disconnect()
{
    // 清空字段指针，确保安全
    _pMysqlFields = nullptr;

    // 如果存在结果集，释放它
    if (_pMysqlRes != nullptr)
    {
        mysql_free_result(_pMysqlRes);
        _pMysqlRes = nullptr;
    }

    // 如果MySQL连接已经打开，关闭它
    if (_pMysql != nullptr)
    {
        mysql_close(_pMysql);
        _pMysql = nullptr;
    }
}

int MysqlBase::CheckMysqlError() const
{
    // 获取MySQL错误代码
    const int mysqlerrno = mysql_errno(_pMysql);
    if (mysqlerrno > 0)
    {
        // 如果有错误，记录详细的错误信息
        LOG_ERROR("MysqlConnector::CheckError. mysql_errno=" << mysql_errno(_pMysql) << ", mysql_error=" << mysql_error(_pMysql));
        return mysqlerrno;
    }

    return 0;
}

bool MysqlBase::Query(const char* sql, my_ulonglong& affected_rows)
{
    // 如果之前有结果集，释放它以避免内存泄漏
    if (nullptr != _pMysqlRes)
    {
        mysql_free_result(_pMysqlRes);
        _pMysqlRes = nullptr;
    }

    // 执行SQL查询
    if (mysql_query(_pMysql, sql) != 0)
    {
        // 查询失败，记录错误并返回false
        LOG_ERROR("Query error:" << mysql_error(_pMysql) << " sql:" << sql);
        return false;
    }

    // 获取查询结果集
    _pMysqlRes = mysql_store_result(_pMysql);
    if (_pMysqlRes != nullptr)
    {
        // 如果结果集非空，获取字段数量和字段描述信息
        _numFields = mysql_num_fields(_pMysqlRes);
        _pMysqlFields = mysql_fetch_fields(_pMysqlRes);
    }

    // 获取查询影响的行数
    affected_rows = mysql_affected_rows(_pMysql);
    return true;
}

MYSQL_ROW MysqlBase::Fetch() const
{
    // 如果没有结果集，返回空指针
    if (_pMysqlRes == nullptr)
        return nullptr;

    // 获取下一行的查询结果
    return mysql_fetch_row(_pMysqlRes);
}

int MysqlBase::GetInt(MYSQL_ROW row, int index)
{
    // 如果该列为空，记录错误日志并返回0
    if (row[index] == nullptr)
    {
        LOG_ERROR("!!! Failed. MysqlConnector::GetInt");
        return 0;
    }

    // 将字符串类型的值转换为整数并返回
    return atoi(row[index]);
}

uint64 MysqlBase::GetUint64(MYSQL_ROW row, int index)
{
    // 如果该列为空，记录错误日志并返回0
    if (row[index] == nullptr)
    {
        LOG_ERROR("!!! Failed. MysqlConnector::GetUint64");
        return 0;
    }

    // 将字符串类型的值转换为无符号64位整数并返回
    return atoll(row[index]);
}

unsigned int MysqlBase::GetUint(MYSQL_ROW row, int index)
{
    // 如果该列为空，记录错误日志并返回0
    if (row[index] == nullptr)
    {
        LOG_ERROR("!!! Failed. MysqlConnector::GetUInt64");
        return 0;
    }

    // 将字符串类型的值转换为无符号整数并返回
    return strtoul(row[index], nullptr, 10);
}

char* MysqlBase::GetString(MYSQL_ROW row, int index)
{
    // 如果该列为空，记录错误日志并返回空指针
    if (row[index] == nullptr)
    {
        LOG_ERROR("!!! Failed. MysqlConnector::GetString");
        return nullptr;
    }

    // 返回字符串值
    return row[index];
}

int MysqlBase::GetBlob(MYSQL_ROW row, int index, char* buf, unsigned long size) const
{
    unsigned int l = size > 0 ? size : 0;

    // 如果该列为空，记录错误日志并返回0
    if (row[index] == nullptr)
    {
        LOG_ERROR("!!! Failed. MysqlConnector::GetBlob");
        return 0;
    }

    // 获取该行每个字段的长度
    unsigned long* lengths = mysql_fetch_lengths(_pMysqlRes);
    if (lengths[index] < l)
        l = lengths[index];

    // 将BLOB数据复制到缓冲区中
    memcpy(buf, row[index], l);
    return l;
}

void MysqlBase::GetBlob(MYSQL_ROW row, int index, std::string& protoStr) const
{
    // 获取每列的长度信息
    unsigned long* pLengths = mysql_fetch_lengths(_pMysqlRes);
    long blobLength = pLengths[index];

    // 如果BLOB长度小于等于0，返回空字符串
    if (blobLength <= 0)
    {
        protoStr = "";
        return;
    }

    // 分配缓冲区并复制BLOB数据
    char* blobByte = new char[blobLength + 1];
    GetBlob(row, index, blobByte, blobLength);
    blobByte[blobLength] = '\0';

    // 将BLOB数据赋值给std::string对象
    protoStr = blobByte;
    delete[] blobByte;  // 释放缓冲区
}
