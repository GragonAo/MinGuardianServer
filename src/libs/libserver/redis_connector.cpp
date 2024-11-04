#include "redis_connector.h"
#include "log4_help.h"
#include "common.h"
#include "util_string.h"
#include "thread_mgr.h"
#include "yaml.h"
#include "component_help.h"

// RedisConnector 类的实现
void RedisConnector::Awake()
{
    // 注册消息处理函数
    RegisterMsgFunction();

    // 获取 YAML 配置，读取数据库管理配置
    auto pYaml = ComponentHelp::GetYaml();
    const auto pConfig = pYaml->GetConfig(APP_DB_MGR);
    auto pDBConfig = dynamic_cast<DBMgrConfig*>(pConfig);

    // 获取 Redis 配置
    auto pRedisConfig = pDBConfig->GetDBConfig(DBMgrConfig::DBTypeRedis);
    if (pRedisConfig == nullptr)
    {
        LOG_ERROR("Init failed. get redis config is failed.");  // 如果配置为空，记录错误日志
        return;
    }

    // 设置 Redis 的 IP 和端口
    _ip = pRedisConfig->Ip;
    _port = pRedisConfig->Port;

    // 设置定时器，每 2 分钟检查一次 Redis 连接状态
    AddTimer(0, 2 * 60, false, 0, BindFunP0(this, &RedisConnector::CheckPing));

    // 尝试连接到 Redis 服务器
    Connect();
}

void RedisConnector::BackToPool()
{
    // 断开与 Redis 的连接
    Disconnect();
}

void RedisConnector::CheckPing()
{
    // 检查 Redis 连接的健康状态
    if (Ping())
        return;  // 如果连接正常，则返回

    // 如果连接失败，尝试断开并重新连接
    Disconnect();
    Connect();
}

bool RedisConnector::Ping() const
{
    // 检查 Redis 上下文是否有效
    if (_pRedisContext == nullptr)
        return false;

    // 向 Redis 发送 PING 命令
    redisReply* pRedisReply = static_cast<redisReply*>(redisCommand(_pRedisContext, "PING"));

    // 如果命令执行失败，记录错误日志
    if (nullptr == pRedisReply)
    {
        LOG_ERROR("RedisConnector::ping: errno=" << _pRedisContext->err << " error=" << _pRedisContext->errstr);
        return false;
    }

    // 检查返回的结果是否为 PONG
    if (!(pRedisReply->type == REDIS_REPLY_STATUS && strncmp(pRedisReply->str, "PONG", 4) == 0))
    {
        LOG_ERROR("RedisConnector::ping: errno=" << _pRedisContext->err << " error=" << _pRedisContext->errstr);
        freeReplyObject(pRedisReply);
        return false;
    }

    // 释放回复对象
    freeReplyObject(pRedisReply);
    return true;  // 返回连接正常
}

bool RedisConnector::Connect()
{
    // 记录连接日志
    LOG_DEBUG("RedisConnector::Connect. " << _ip.c_str() << ":" << _port << ". starting...");

    // 设置连接超时为 1.5 秒
    const struct timeval outTime = { 1, 500000 };
    redisContext* c = redisConnectWithTimeout(_ip.c_str(), _port, outTime);
    
    // 检查连接是否成功
    if (c == nullptr || c->err)
    {
        if (c)
        {
            LOG_ERROR("RedisConnector::Connect. errno=" << c->err << ", error=" << c->errstr);
            redisFree(c);  // 释放无效的 Redis 上下文
        }
        else
        {
            LOG_ERROR("RedisConnector error: can't allocate redis context");
        }
        return false;
    }

    // 选择 Redis 数据库 (选择数据库索引为 1)
    int db_index = 1;
    std::string sql = strutil::format("select %d", db_index);
    redisReply* pRedisReply = static_cast<redisReply*>(redisCommand(c, sql.c_str()));

    // 检查选择数据库命令的执行结果
    if (nullptr == pRedisReply)
    {
        LOG_ERROR("RedisConnector::Connect. errno=" << c->err << ", error=" << c->errstr);
        redisFree(c);  // 释放无效的 Redis 上下文
        return false;
    }

    // 检查返回的结果是否为 OK
    if (!(pRedisReply->type == REDIS_REPLY_STATUS && strncmp(pRedisReply->str, "OK", 2) == 0))
    {
        LOG_ERROR("RedisConnector::Connect. errno=" << c->err << ", error=" << c->errstr);
        freeReplyObject(pRedisReply);  // 释放回复对象
        redisFree(c);  // 释放无效的 Redis 上下文
        return false;
    }

    freeReplyObject(pRedisReply);  // 释放回复对象
    _pRedisContext = c;  // 保存 Redis 上下文

    LOG_DEBUG("\tRedisConnector::Connect: successfully!");  // 记录成功连接日志
    return Ping();  // 检查连接是否正常
}

bool RedisConnector::Disconnect()
{
    // 断开 Redis 连接
    if (_pRedisContext != nullptr)
    {
        redisFree(_pRedisContext);  // 释放 Redis 上下文
        _pRedisContext = nullptr;  // 清空指针
    }
    return true;
}

bool RedisConnector::Setex(std::string key, std::string value, const int timeout) const
{
    // 使用 SETEX 命令设置键值及过期时间
    const std::string command = strutil::format("SETEX %s %d %s", key.c_str(), timeout, value.c_str());
    return Setex(command);  // 调用 Setex 处理命令
}

bool RedisConnector::Setex(std::string key, const uint64 value, const int timeout) const
{
    // 重载函数，使用 SETEX 命令设置键值及过期时间，支持 uint64 类型
    const std::string command = strutil::format("SETEX %s %d %llu", key.c_str(), timeout, value);
    return Setex(command);  // 调用 Setex 处理命令
}

bool RedisConnector::Setex(std::string command) const
{
    // 执行 SETEX 命令
    redisReply* pRedisReply = static_cast<redisReply*>(redisCommand(_pRedisContext, command.c_str()));
    if (pRedisReply->type != REDIS_REPLY_STATUS || strncmp(pRedisReply->str, "OK", 2) != 0)
    {
        LOG_WARN("[SETEX] failed. command:" << command.c_str() << " pRedisReply->type:" << pRedisReply->type << " pRedisReply->str:" << pRedisReply->str);
        freeReplyObject(pRedisReply);  // 释放回复对象
        return false;  // 返回失败
    }

#if LOG_REDIS_OPEN
    LOG_REDIS(" @@ SETEX. key:" << command.c_str());  // 日志记录
#endif

    freeReplyObject(pRedisReply);  // 释放回复对象
    return true;  // 返回成功
}

bool RedisConnector::SetnxExpire(std::string key, const int value, const int timeout) const
{
    // 检查 Redis 上下文是否有效
    if (_pRedisContext == nullptr)
    {
        LOG_WARN("RedisContext == nullptr. connect failed.");  // 如果上下文为空，记录警告日志
        return false;  // 返回失败
    }

    // 构造 SETNX 命令，尝试设置键值
    std::string command = strutil::format("SETNX %s %d", key.c_str(), value);
    redisReply* pRedisReply = static_cast<redisReply*>(redisCommand(_pRedisContext, command.c_str()));

    // 检查 SETNX 命令的执行结果
    if (pRedisReply->type != REDIS_REPLY_INTEGER || pRedisReply->integer != 1)
    {
        // 如果设置失败，记录警告日志，表示该键已经存在
        LOG_WARN("[SETNX] failed 1. command:" << command.c_str() << " pRedisReply->type:" << pRedisReply->type << " pRedisReply->integer:" << pRedisReply->integer);
        freeReplyObject(pRedisReply);  // 释放回复对象
        return false;  // 返回失败
    }

#if LOG_REDIS_OPEN
    LOG_REDIS(" @@ SETNX. key:" << command.c_str());  // 如果成功，记录日志
#endif

    freeReplyObject(pRedisReply);  // 释放回复对象

    // 成功后为该键设置过期时间
    command = strutil::format("EXPIRE %s %d", key.c_str(), timeout);
    pRedisReply = static_cast<redisReply*>(redisCommand(_pRedisContext, command.c_str()));

    // 检查 EXPIRE 命令的执行结果
    if (pRedisReply->type != REDIS_REPLY_INTEGER || pRedisReply->integer != 1)
    {
        // 如果设置过期时间失败，记录警告日志
        LOG_WARN("[EXPIRE] failed 2. command:" << command.c_str() << " pRedisReply->type:" << pRedisReply->type << " pRedisReply->integer:" << pRedisReply->integer);
        freeReplyObject(pRedisReply);  // 释放回复对象
        return false;  // 返回失败
    }

#if LOG_REDIS_OPEN
    LOG_REDIS(" @@ EXPIRE. key:" << command.c_str());  // 记录设置过期时间的日志
#endif

    freeReplyObject(pRedisReply);  // 释放回复对象
    return true;  // 返回成功
}

void RedisConnector::Delete(std::string key) const
{
    // 构造 DEL 命令，尝试删除指定键
    std::string command = strutil::format("DEL %s", key.c_str());
    redisReply* pRedisReply = static_cast<redisReply*>(redisCommand(_pRedisContext, command.c_str()));

    // 检查 DEL 命令的执行结果
    if (pRedisReply->type != REDIS_REPLY_INTEGER)
    {
        LOG_ERROR("[DEL] execute command:" << command.c_str() << " pRedisReply->type:" << pRedisReply->type);  // 记录错误日志
    }

    if (pRedisReply->integer == 0)
    {
        // 如果没有任何键被删除，可以记录日志（这里的日志已注释）
        // LOG_COLOR(LogColorPurple, "[DEL] failed. command:" << command.c_str() << ". nothing is deleted. size = 0.");
    }
    else
    {
#if LOG_REDIS_OPEN
        LOG_REDIS(" @@ DEL. key:" << command.c_str());  // 如果成功，记录日志
#endif
    }

    freeReplyObject(pRedisReply);  // 释放回复对象
}

int RedisConnector::GetInt(std::string key) const
{
    int nRs = 0;  // 存储结果的变量
    // 构造 GET 命令，尝试获取指定键的整数值
    std::string command = strutil::format("GET %s", key.c_str());
    redisReply* pRedisReply = static_cast<redisReply*>(redisCommand(_pRedisContext, command.c_str()));

    // 检查 GET 命令的执行结果
    if (pRedisReply->type != REDIS_REPLY_INTEGER && pRedisReply->type != REDIS_REPLY_STRING)
    {
        // LOG_ERROR("[GET failed] execute command:" << command.c_str() << " pRedisReply->type:" << pRedisReply->type);  // 记录错误日志
    }

    // 根据返回类型获取值
    if (pRedisReply->type == REDIS_REPLY_INTEGER)
    {
        nRs = static_cast<int>(pRedisReply->integer);  // 如果返回的是整数类型，直接赋值
    }
    else if (pRedisReply->type == REDIS_REPLY_STRING)
    {
        nRs = std::stoi(pRedisReply->str, nullptr, 10);  // 如果返回的是字符串类型，转换为整数
    }

    freeReplyObject(pRedisReply);  // 释放回复对象
    return nRs;  // 返回结果
}

std::string RedisConnector::GetString(std::string key) const
{
    std::string strRs = "";  // 存储结果的变量
    // 构造 GET 命令，尝试获取指定键的字符串值
    std::string command = strutil::format("GET %s", key.c_str());
    redisReply* pRedisReply = static_cast<redisReply*>(redisCommand(_pRedisContext, command.c_str()));

    // 检查 GET 命令的执行结果
    if (pRedisReply->type != REDIS_REPLY_STRING)
    {
        LOG_WARN("[GET failed] execute command:" << command.c_str() << " pRedisReply->type:" << pRedisReply->type);  // 记录警告日志
    }
    else
    {
        strRs = pRedisReply->str;  // 如果成功，获取字符串值
    }

    freeReplyObject(pRedisReply);  // 释放回复对象
    return strRs;  // 返回结果
}

uint64 RedisConnector::GetUint64(std::string key) const
{
    uint64 rsValue = 0;  // 存储结果的变量
    // 构造 GET 命令，尝试获取指定键的 uint64 值
    std::string command = strutil::format("GET %s", key.c_str());
    redisReply* pRedisReply = static_cast<redisReply*>(redisCommand(_pRedisContext, command.c_str()));

    // 检查 GET 命令的执行结果
    if (pRedisReply->type != REDIS_REPLY_INTEGER && pRedisReply->type != REDIS_REPLY_STRING)
    {
        LOG_WARN("[GET failed] execute command:" << command.c_str() << " pRedisReply->type:" << pRedisReply->type);  // 记录警告日志
    }

    // 根据返回类型获取值
    if (pRedisReply->type == REDIS_REPLY_INTEGER)
    {
        rsValue = pRedisReply->integer;  // 如果返回的是整数类型，直接赋值
    }
    else if (pRedisReply->type == REDIS_REPLY_STRING)
    {
        rsValue = std::stoll(pRedisReply->str, nullptr, 10);  // 如果返回的是字符串类型，转换为 uint64
    }

    freeReplyObject(pRedisReply);  // 释放回复对象
    return rsValue;  // 返回结果
}
