#include "mysql_connector.h"
#include "libserver/log4_help.h"
#include "libserver/component_help.h"

#include <thread>

// 定义MySQL ping操作的间隔时间（秒）
#define MysqlPingTime 2 * 60

// 初始化MySQL连接器，加载配置并连接数据库
void MysqlConnector::Awake()
{
    // 初始化MySQL线程环境
    mysql_thread_init();

    LOG_DEBUG("MysqlConnector::Awake. id:" << std::this_thread::get_id());

    // 获取YAML配置文件对象
    auto pYaml = ComponentHelp::GetYaml();
    // 从配置文件中获取数据库管理器的配置
    auto pConfig = pYaml->GetConfig(APP_DB_MGR);
    // 强制转换为DBMgrConfig类型
    auto pDbCfig = dynamic_cast<DBMgrConfig*>(pConfig);

    // 获取MySQL数据库的配置信息
    _pDbCfg = pDbCfig->GetDBConfig(DBMgrConfig::DBTypeMysql);
    if (_pDbCfg == nullptr)
    {
        LOG_ERROR("Failed to get mysql config.");
        return;
    }

    // 初始化消息组件并建立数据库连接
    InitMessageComponent();
    Connect();

    // 定时检查MySQL连接的ping操作
    // AddTimeHeapFunction(0, MysqlPingTime, false, 0, BindFunP0(this, &MysqlConnector::CheckPing));
}

// 定时检查MySQL连接状态，如果连接断开则尝试重连
void MysqlConnector::CheckPing()
{
    // 检查MySQL连接的ping状态
    if (_isRunning)
    {
        mysql_ping(_pMysql);
        if (CheckMysqlError() != 0)
        {
            // 如果检测到错误，断开连接
            Disconnect();
        }
    }

    // 如果连接已断开，则重新连接
    if (!_isRunning)
    {
        ReConnect();
    }
}

// 将连接器资源返回到对象池，关闭连接并释放线程环境
void MysqlConnector::BackToPool()
{
    Disconnect();
    mysql_thread_end();
}

// 建立与MySQL数据库的连接
bool MysqlConnector::Connect()
{
    if (!ConnectInit())
        return false;

    LOG_DEBUG("MysqlConnector::Connect. " << _pDbCfg->Ip.c_str() << ":" << _pDbCfg->Port << " starting... id:" << std::this_thread::get_id());

    // 连接到MySQL数据库
    mysql_real_connect(_pMysql, _pDbCfg->Ip.c_str(), _pDbCfg->User.c_str(), _pDbCfg->Password.c_str(), _pDbCfg->DatabaseName.c_str(), _pDbCfg->Port, nullptr, CLIENT_FOUND_ROWS);

    // 检查MySQL连接是否成功
    int mysqlerrno = CheckMysqlError();
    if (mysqlerrno > 0)
    {
        Disconnect();
        return false;
    }

    // 禁用自动提交（如果需要手动事务处理）
    // mysql_autocommit(mysql(), 0);

    // 检查MySQL连接的ping状态
    mysql_ping(_pMysql);
    mysqlerrno = CheckMysqlError();
    if (mysqlerrno > 0)
    {
        Disconnect();
        return false;
    }

    LOG_DEBUG("MysqlConnector::Connect: successfully! addr:" << _pDbCfg->Ip.c_str() << ":" << _pDbCfg->Port);

    // 初始化预处理语句
    InitStmts();

    // 设置连接状态为运行中
    _isRunning = true;

    return true;
}

// 尝试重新连接到MySQL数据库
void MysqlConnector::ReConnect()
{
    if (!ConnectInit())
        return;

    LOG_DEBUG("MysqlConnector::ReConnect. " << _pDbCfg->Ip.c_str() << ":" << _pDbCfg->Port << " starting...");

    // 重连数据库
    if (nullptr == mysql_real_connect(_pMysql, _pDbCfg->Ip.c_str(), _pDbCfg->User.c_str(), _pDbCfg->Password.c_str(), _pDbCfg->DatabaseName.c_str(), _pDbCfg->Port, nullptr, CLIENT_FOUND_ROWS))
    {
        LOG_ERROR("MysqlConnector::ReConnect failed. " << _pDbCfg->Ip.c_str() << ":" << _pDbCfg->Port << " starting...");
        CheckMysqlError();
        return;
    }

    // 选择数据库
    mysql_select_db(_pMysql, _pDbCfg->DatabaseName.c_str());
    if (CheckMysqlError() != 0)
    {
        Disconnect();
        return;
    }

    LOG_DEBUG("MysqlConnector::ReConnect: successfully! addr:" << _pDbCfg->Ip.c_str() << ":" << _pDbCfg->Port);

    // 初始化预处理语句
    InitStmts();

    // 设置连接状态为运行中
    _isRunning = true;
}

// 断开与MySQL数据库的连接并清理预处理语句
void MysqlConnector::Disconnect()
{
    CleanStmts();
    MysqlBase::Disconnect();
}

// 初始化预处理语句
void MysqlConnector::InitStmts()
{
    // 创建插入玩家数据的预处理语句
    DatabaseStmt* stmt = CreateStmt("insert into player ( sn, account, name, savetime, createtime ) value ( ?, ?, ?, now(), now() )");
    _mapStmt.insert(std::make_pair(DatabaseStmtKey::StmtCreate, stmt));

    // 创建更新玩家数据的预处理语句
    stmt = CreateStmt("update player set base=?, misc=?,savetime=now() where sn = ?");
    _mapStmt.insert(std::make_pair(DatabaseStmtKey::StmtSave, stmt));

    LOG_DEBUG("\tMysqlConnector::InitStmts successfully!");
}

// 清理所有预处理语句并释放相关资源
void MysqlConnector::CleanStmts()
{
    for (auto one : _mapStmt)
    {
        one.second->Close();
        delete one.second;
    }

    _mapStmt.clear();
}

// 根据语句键获取相应的预处理语句
DatabaseStmt* MysqlConnector::GetStmt(DatabaseStmtKey stmtKey)
{
    const auto iter = _mapStmt.find(stmtKey);
    if (iter == _mapStmt.end())
        return nullptr;

    return iter->second;
}

// 创建新的预处理语句
DatabaseStmt* MysqlConnector::CreateStmt(const char* sql) const
{
    int str_len = strlen(sql);
    DatabaseStmt* stmt = new DatabaseStmt();
    int param_count = 0;
    stmt->stmt = mysql_stmt_init(_pMysql);
    if (mysql_stmt_prepare(stmt->stmt, sql, str_len) != 0)
    {
        LOG_ERROR("create_stmt error, " << LOG4CPLUS_C_STR_TO_TSTRING(mysql_stmt_error(stmt->stmt)));
        return nullptr;
    }

    // 计算SQL语句中的参数个数
    for (int i = 0; i < str_len; i++)
    {
        if ((sql[i] == '?') || (sql[i] == '@'))
            param_count++;
    }

    // 如果存在参数，则为其分配绑定资源
    if (param_count > 0)
    {
        stmt->bind = new MYSQL_BIND[param_count];
        memset(stmt->bind, 0, sizeof(MYSQL_BIND) * param_count);
        stmt->bind_buffer = new char[MAX_BIND_BUFFER];
    }
    else
    {
        stmt->bind = nullptr;
        stmt->bind_buffer = nullptr;
    }

    return stmt;
}

// 清除预处理语句的参数绑定信息
void MysqlConnector::ClearStmtParam(DatabaseStmt* stmt)
{
    stmt->bind_index = 0;
    stmt->bind_buffer_index = 0;
}

// 执行预处理语句，并获取影响的行数
bool MysqlConnector::ExecuteStmt(DatabaseStmt* stmt, my_ulonglong& affected_rows)
{
    MYSQL_STMT* st = stmt->stmt;
    if (NULL != stmt->bind)
        mysql_stmt_bind_param(stmt->stmt, stmt->bind);
    if (mysql_stmt_execute(st) != 0)
    {
        LOG_ERROR("execute_stmt error, " << LOG4CPLUS_C_STR_TO_TSTRING(mysql_stmt_error(st)));
        return false;
    }

    affected_rows = mysql_stmt_affected_rows(st);
    return true;
}

// 执行预处理语句（不关心影响行数）
bool MysqlConnector::ExecuteStmt(DatabaseStmt* stmt)
{
    MYSQL_STMT* st = stmt->stmt;
    if (NULL != stmt->bind)
        mysql_stmt_bind_param(stmt->stmt, stmt->bind);
    if (mysql_stmt_execute(st) != 0)
    {
        LOG_ERROR("execute_stmt error, " << LOG4CPLUS_C_STR_TO_TSTRING(mysql_stmt_error(st)));
        return false;
    }

    return true;
}

// 为预处理语句添加BLOB类型的参数
void MysqlConnector::AddParamBlob(DatabaseStmt* stmt, void* val, int size)
{
    // 获取当前语句绑定的下一个可用绑定对象
    MYSQL_BIND* pBind = &stmt->bind[stmt->bind_index];

    // 指定参数的类型为 BLOB（即二进制大对象）
    pBind->buffer_type = MYSQL_TYPE_BLOB;

    // 设置参数的值，直接使用传入的二进制数据指针
    pBind->buffer = val;

    // 将数据长度指针设置为绑定缓冲区中的一个位置
    pBind->length = reinterpret_cast<unsigned long*>(&stmt->bind_buffer[stmt->bind_buffer_index + sizeof(void*)]);

    // 设置该长度值为传入的二进制数据大小
    *(pBind->length) = size;

    // 设置缓冲区的长度为传入数据的大小
    pBind->buffer_length = size;

    // 更新绑定索引，准备绑定下一个参数
    stmt->bind_index++;

    // 绑定缓冲区索引也相应增加，为下一个参数预留空间
    stmt->bind_buffer_index += (sizeof(void*) + sizeof(unsigned long*));
}

void MysqlConnector::AddParamUint64(DatabaseStmt* stmt, uint64 val)
{
    // 获取当前语句绑定的下一个可用绑定对象
    MYSQL_BIND* pBind = &stmt->bind[stmt->bind_index];

    // 指定参数类型为 64 位长整型
    pBind->buffer_type = MYSQL_TYPE_LONGLONG;

    // 设置缓冲区指针，存储无符号 64 位整数的值
    pBind->buffer = &stmt->bind_buffer[stmt->bind_buffer_index];

    // 指定该参数为无符号类型
    pBind->is_unsigned = true;

    // 将传入的 64 位无符号整数值赋给缓冲区
    *static_cast<uint64*>(pBind->buffer) = val;

    // 更新绑定索引，准备绑定下一个参数
    stmt->bind_index++;

    // 绑定缓冲区索引也相应增加，为下一个参数预留空间
    stmt->bind_buffer_index += sizeof(uint64);
}

void MysqlConnector::AddParamInt(DatabaseStmt* stmt, int val)
{
    // 获取当前语句绑定的下一个可用绑定对象
    MYSQL_BIND* pBind = &stmt->bind[stmt->bind_index];

    // 指定参数类型为 32 位整型
    pBind->buffer_type = MYSQL_TYPE_LONG;

    // 设置缓冲区指针，存储有符号整数的值
    pBind->buffer = &stmt->bind_buffer[stmt->bind_buffer_index];

    // 指定该参数为有符号类型
    pBind->is_unsigned = false;

    // 将传入的整数值赋给缓冲区
    *static_cast<long*>(pBind->buffer) = val;

    // 更新绑定索引，准备绑定下一个参数
    stmt->bind_index++;

    // 绑定缓冲区索引也相应增加，为下一个参数预留空间
    stmt->bind_buffer_index += sizeof(long);
}

void MysqlConnector::AddParamStr(DatabaseStmt* stmt, const char* val)
{
    // 获取当前语句绑定的下一个可用绑定对象
    MYSQL_BIND* pBind = &stmt->bind[stmt->bind_index];

    // 获取字符串长度
    size_t len = strlen(val);

    // 检查字符串长度是否超过允许的最大长度
    if (len >= MAX_BIND_STR)
    {
        // 如果超过最大长度，记录错误日志，并将长度截断到最大允许长度减 1
        LOG_ERROR("add_str_param error, str size:" << len << " out of limit:" << MAX_BIND_STR);
        len = MAX_BIND_STR - 1;
    }

    // 设置参数类型为字符串
    pBind->buffer_type = MYSQL_TYPE_STRING;

    // 设置缓冲区指针，存储字符串值
    pBind->buffer = &stmt->bind_buffer[stmt->bind_buffer_index];

    // 将数据长度的指针设置为绑定缓冲区中字符串后的一个位置
    pBind->length = (unsigned long*)&stmt->bind_buffer[stmt->bind_buffer_index + len + 1];

    // 复制传入的字符串到缓冲区，确保其长度不超过指定的长度
    engine_strncpy((char*)pBind->buffer, len + 1, val, len + 1);

    // 将字符串长度赋值给长度字段
    *(pBind->length) = len;

    // 设置缓冲区的总长度为字符串的实际长度
    pBind->buffer_length = len;

    // 更新绑定索引，准备绑定下一个参数
    stmt->bind_index++;

    // 绑定缓冲区索引也相应增加，为下一个参数预留空间
    stmt->bind_buffer_index += (len + 1 + sizeof(unsigned long*));
}

