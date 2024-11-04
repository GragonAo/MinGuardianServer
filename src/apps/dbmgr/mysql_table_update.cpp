#include "mysql_table_update.h"

#include "libserver/log4_help.h"
#include "libserver/util_string.h"
#include "libserver/component_help.h"

#include <mysql/mysql.h>
#include <mysql/mysqld_error.h>
#include <thread>

MysqlTableUpdate::MysqlTableUpdate()
{
    // 在构造函数中，添加更新函数到向量 _update_func 中。更新函数按照顺序依次执行。
    _update_func.push_back(BindFunP0(this, &MysqlTableUpdate::Update00)); 
}

MysqlTableUpdate::~MysqlTableUpdate()
{
    // 析构函数：断开与数据库的连接
    Disconnect();
}

void MysqlTableUpdate::Check()
{
    // 检查数据库连接是否初始化
    if (!ConnectInit())
        return;

    // 从配置文件中获取数据库配置
    auto pYaml = ComponentHelp::GetYaml();
    auto pDbMgrCfig = dynamic_cast<DBMgrConfig*>(pYaml->GetConfig(APP_DB_MGR));
    _pDbCfg = pDbMgrCfig->GetDBConfig(DBMgrConfig::DBTypeMysql);
    if (_pDbCfg == nullptr)
    {
        LOG_ERROR("初始化失败，无法获取 MySQL 配置");
        return;
    }

    LOG_DEBUG("MySQL 更新连接：" << _pDbCfg->Ip.c_str() << ":" << _pDbCfg->Port << " 启动中... 线程ID：" << std::this_thread::get_id());

    // 连接到 MySQL 数据库
    if (mysql_real_connect(_pMysql, _pDbCfg->Ip.c_str(), _pDbCfg->User.c_str(), _pDbCfg->Password.c_str(), nullptr, _pDbCfg->Port, nullptr, CLIENT_FOUND_ROWS))
    {
        // 选择指定的数据库
        mysql_select_db(_pMysql, _pDbCfg->DatabaseName.c_str());
    }

    // 检查 MySQL 错误
    int mysqlerrno = CheckMysqlError();
    if (mysqlerrno == ER_BAD_DB_ERROR)
    {
        LOG_DEBUG("MySQL 尝试创建数据库：" << _pDbCfg->DatabaseName.c_str());

        // 错误码1049表示数据库不存在，尝试创建数据库
        if (!CreateDatabaseIfNotExist())
        {
            Disconnect();
            return;
        }

        // 再次选择数据库
        mysql_select_db(_pMysql, _pDbCfg->DatabaseName.c_str());
        mysqlerrno = CheckMysqlError();
    }

    // 如果仍有错误，则断开连接
    if (mysqlerrno > 0)
    {
        Disconnect();
        return;
    }

    // 数据库版本更新
    if (!UpdateToVersion())
    {
        LOG_ERROR("MySQL 更新失败：UpdateToVersion");
        return;
    }

    // 保持连接状态
    mysql_ping(_pMysql);
    mysqlerrno = CheckMysqlError();
    if (mysqlerrno > 0)
    {
        Disconnect();
        return;
    }

    LOG_DEBUG("MySQL 更新成功！ 地址：" << _pDbCfg->Ip.c_str() << ":" << _pDbCfg->Port);
}

bool MysqlTableUpdate::CreateDatabaseIfNotExist()
{
    // 构建创建数据库的 SQL 语句，如果数据库不存在则创建
    std::string querycmd = strutil::format("CREATE DATABASE IF NOT EXISTS %s;", _pDbCfg->DatabaseName.c_str());

    my_ulonglong affected_rows;
    if (!Query(querycmd.c_str(), affected_rows)) {
        LOG_ERROR("!!! 失败：无法创建数据库：" << querycmd.c_str());
        return false;
    }

    // 选择指定数据库
    if (mysql_select_db(_pMysql, _pDbCfg->DatabaseName.c_str()) != 0) {
        LOG_ERROR("!!! 失败：无法选择数据库：" << LOG4CPLUS_STRING_TO_TSTRING(_pDbCfg->DatabaseName));
        return false;
    }

    // 设置字符集
    if (mysql_set_character_set(_pMysql, _pDbCfg->CharacterSet.c_str()) != 0) {
        LOG_ERROR("!!! 失败：无法设置字符集：" << LOG4CPLUS_STRING_TO_TSTRING(_pDbCfg->CharacterSet));
        return false;
    }

    // 设置数据库的字符集和排序规则
    querycmd = strutil::format("ALTER DATABASE CHARACTER SET %s COLLATE %s", _pDbCfg->CharacterSet.c_str(), _pDbCfg->Collation.c_str());
    if (!Query(querycmd.c_str(), affected_rows)) {
        LOG_ERROR("!!! 失败：无法修改数据库字符集和排序规则：" << LOG4CPLUS_STRING_TO_TSTRING(querycmd.c_str()));
        return false;
    }

    // 创建 version 表用于记录数据库版本
    std::string create_version =
        "CREATE TABLE IF NOT EXISTS `version` (" \
        "`version` int(11) NOT NULL," \
        "PRIMARY KEY (`version`)" \
        ") ENGINE=%s DEFAULT CHARSET=%s;";

    std::string cmd = strutil::format(create_version.c_str(), "InnoDB", _pDbCfg->CharacterSet.c_str());
    if (!Query(cmd.c_str(), affected_rows)) {
        LOG_ERROR("!!! 失败：无法创建 version 表：" << LOG4CPLUS_STRING_TO_TSTRING(cmd));
        return false;
    }

    // 创建 player 表，用于存储玩家信息
    std::string create_player =
        "CREATE TABLE IF NOT EXISTS `player` (" \
        "`sn` bigint(20) NOT NULL," \
        "`name` char(32) NOT NULL," \
        "`account` char(64) NOT NULL," \
        "`base` blob," \
        "`item` blob,"  \
        "`misc` blob,"  \
        "`savetime` datetime default NULL," \
        "`createtime` datetime default NULL," \
        "PRIMARY KEY  (`sn`),"  \
        "UNIQUE KEY `NAME` (`name`),"  \
        "KEY `ACCOUNT` (`account`)"  \
        ") ENGINE=%s DEFAULT CHARSET=%s;";

    cmd = strutil::format(create_player.c_str(), "InnoDB", _pDbCfg->CharacterSet.c_str());
    if (!Query(cmd.c_str(), affected_rows)) {
        LOG_ERROR("!!! 失败：无法创建 player 表：" << LOG4CPLUS_STRING_TO_TSTRING(cmd));
        return false;
    }

    // 插入初始 version 记录，表示数据库版本为 0
    cmd = "insert into `version` VALUES ('0')";
    if (!Query(cmd.c_str(), affected_rows)) {
        LOG_ERROR("!!! 失败：无法插入初始版本号：" << LOG4CPLUS_STRING_TO_TSTRING(cmd));
        return false;
    }

    return true;
}

bool MysqlTableUpdate::UpdateToVersion()
{
    // 获取当前数据库版本号
    my_ulonglong affected_rows;
    std::string sql = "select version from `version`";
    if (!Query(sql.c_str(), affected_rows))
        return false;

    MYSQL_ROW row = Fetch();
    if (row == nullptr)
        return false;

    int version = GetInt(row, 0);
    if (version == _version)
        return true;

    // 依次执行更新函数，将数据库版本逐步更新到最新版本
    for (int i = version + 1; i <= _version; i++) {
        if (_update_func[i] == nullptr)
            continue;

        if (!_update_func[i]()) {
            LOG_ERROR("更新至版本 " << i << " 失败");
            return false;
        }

        LOG_INFO("数据库更新至版本：" << i);

        // 更新成功后，修改数据库中的版本号
        std::string cmd = strutil::format("update `version` set version = %d", i);
        if (!Query(cmd.c_str(), affected_rows)) {
            LOG_ERROR("更新版本号失败，版本：" << i);
            return false;
        }
    }

    return true;
}

bool MysqlTableUpdate::Update00()
{
    // 版本 00 的更新操作（这里没有具体内容）
    return true;
}
