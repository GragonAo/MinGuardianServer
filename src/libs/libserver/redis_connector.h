#pragma once

#include "entity.h"
#include "system.h"
#include <hiredis/hiredis.h>

// RedisConnector 类用于管理 Redis 连接以及与 Redis 进行交互
class RedisConnector : public Entity<RedisConnector>, public IAwakeSystem<>
{
public:
    // 重载的 Awake 方法，用于初始化 Redis 连接
    void Awake() override;
    
    // 将对象返回到对象池中（清理相关资源）
    void BackToPool() override;

    // 连接到 Redis 服务器
    bool Connect();
    
    // 断开与 Redis 的连接
    bool Disconnect();

protected:
    // 注册消息处理函数的虚函数，子类需实现该方法来处理具体的消息
    virtual void RegisterMsgFunction() = 0;

    // 向 Redis 发送 Ping 请求，检查连接是否正常
    bool Ping() const;

    // 检查 Redis 连接并定期发送 Ping 保持连接
    void CheckPing();

    // 将键值对存入 Redis，并设置超时时间
    bool Setex(std::string key, std::string value, int timeout) const;

    // 将键值对存入 Redis，并设置超时时间 (uint64 类型)
    bool Setex(std::string key, uint64 value, int timeout) const;

    // 尝试在 Redis 中设置键值对，如果键不存在则设置，并设置超时时间
    bool SetnxExpire(std::string key, int value, int timeout) const;

    // 从 Redis 中删除指定的键
    void Delete(std::string key) const;

    // 获取 Redis 中指定键的整数值
    int GetInt(std::string key) const;

    // 获取 Redis 中指定键的字符串值
    std::string GetString(std::string key) const;

    // 获取 Redis 中指定键的 uint64 值
    uint64 GetUint64(std::string key) const;

private:
    // 将带有超时时间的键值对存入 Redis
    bool Setex(std::string command) const;

protected:
    // hiredis 的 redisContext 对象，用于维护 Redis 连接
    redisContext* _pRedisContext{ nullptr };

    // Redis 服务器 IP 地址（默认是本地）
    std::string _ip{ "127.0.0.1" };

    // Redis 服务器端口号（默认是 8080）
    int _port{ 8080 };
};
