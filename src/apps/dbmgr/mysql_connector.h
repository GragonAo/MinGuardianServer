#pragma once

#include "libserver/common.h"
#include "libserver/yaml.h"
#include "libserver/util_time.h"
#include "libserver/entity.h"
#include "libserver/system.h"
#include "libserver/socket_object.h"

#include "mysql_base.h"

#include <mysql/mysql.h>
#include <vector>

class Packet;

enum DatabaseStmtKey
{
    StmtCreate, // 用于创建的预处理语句
    StmtSave,   // 用于保存的预处理语句
};

struct DatabaseStmt
{
    MYSQL_STMT *stmt{ nullptr };  // 预处理语句指针
    MYSQL_BIND *bind{ nullptr };  // 参数绑定指针
    char *bind_buffer{ nullptr }; // 用于保存绑定数据的缓冲区
    int bind_index;               // 当前绑定参数的索引
    int bind_buffer_index;        // 当前绑定缓冲区的索引

    // 关闭并释放预处理语句和相关资源
    void Close()
    {
        if (bind != nullptr) 
        {
            delete[] bind;
            bind = nullptr;
        }

        if (bind_buffer != nullptr) 
        {
            delete[] bind_buffer;
            bind_buffer = nullptr;
        }

        if (stmt != nullptr) 
        {
            mysql_stmt_close(stmt);
            stmt = nullptr;
        }
    }
};

// 定义最大绑定缓冲区和字符串的大小
#define MAX_BIND_BUFFER    40960
#define MAX_BIND_STR       30000

class MysqlConnector : public MysqlBase, public Entity<MysqlConnector>, public IAwakeSystem<>
{
public:
    // 初始化组件，调用父类的Awake方法
    void Awake() override;

    // 重置资源并将对象返回对象池
    void BackToPool() override;

    // 初始化消息组件
    void InitMessageComponent();

    // 连接到数据库
    bool Connect();

    // 断开与数据库的连接，重写父类的Disconnect方法
    void Disconnect() override;

private:
    // 尝试重新连接到数据库
    void ReConnect();

    // 检查MySQL连接的有效性，通过ping操作保持连接活跃
    void CheckPing();

#pragma region 预处理语句

    // 初始化预处理语句
    void InitStmts();

    // 清理预处理语句并释放资源
    void CleanStmts();

    // 根据键值获取预处理语句
    DatabaseStmt* GetStmt(DatabaseStmtKey stmtKey);

    // 创建新的预处理语句
    DatabaseStmt* CreateStmt(const char *sql) const;

    // 清除预处理语句的绑定参数
    void ClearStmtParam(DatabaseStmt* stmt);

    // 为预处理语句添加字符串参数
    static void AddParamStr(DatabaseStmt* stmt, const char* value);

    // 为预处理语句添加整数参数
    static void AddParamInt(DatabaseStmt* stmt, int val);

    // 为预处理语句添加无符号64位整数参数
    static void AddParamUint64(DatabaseStmt* stmt, uint64 val);

    // 为预处理语句添加BLOB类型参数
    static void AddParamBlob(DatabaseStmt* stmt, void *val, int size);

    // 执行预处理语句并获取影响的行数
    bool ExecuteStmt(DatabaseStmt* stmt, my_ulonglong& affected_rows);

    // 执行预处理语句
    bool ExecuteStmt(DatabaseStmt* stmt);

#pragma endregion 

    // 协议处理 - 查询玩家列表
    void HandleQueryPlayerList(Packet* pPacket);

    // 根据账号查询玩家列表
    void QueryPlayerList(std::string account, NetIdentify* pIdentify);

    // 协议处理 - 查询单个玩家
    void HandleQueryPlayer(Packet* pPacket);

    // 协议处理 - 创建玩家
    void HandleCreatePlayer(Packet* pPacket);

    // 协议处理 - 保存玩家数据
    void HandleSavePlayer(Packet* pPacket);

    // 保存玩家数据到数据库
    bool OnSavePlayer(DatabaseStmt* stmtSave, Proto::Player& protoPlayer);

protected:
    // 存储所有预处理语句的映射，键为预定义的语句类型
    std::map<DatabaseStmtKey, DatabaseStmt*> _mapStmt;

    // 标志位，标识数据库连接是否正在运行
    bool _isRunning{ false };
};
