#pragma once

#include "common.h"
#include "app_type.h"
#include "component.h"
#include "system.h"

#include <yaml-cpp/yaml.h>

// 基础配置结构体
struct YamlConfig
{
    virtual ~YamlConfig() {} // 虚析构函数，确保派生类能正确释放资源
};

// 应用程序配置结构体
struct AppConfig : public YamlConfig
{
    int LogicThreadNum{ 0 };      // 逻辑线程数量
    int MysqlThreadNum{ 0 };       // MySQL 线程数量
    int ListenThreadNum{ 1 };      // 监听线程数量
    int ConnectThreadNum{ 1 };     // 连接线程数量
};

// 通用配置结构体，继承自 AppConfig
struct CommonConfig : public AppConfig
{
    std::string Ip{ "127.0.0.1" }; // 默认 IP 地址
    int Port{ 6661 };               // 默认端口
    int HttpPort{ 5051 };           // HTTP 默认端口
};

// 单个应用程序配置，继承自 CommonConfig
struct AppListForOneConfig : public CommonConfig
{
    int Id{ 0 }; // 应用程序 ID
};

// 应用程序列表配置，继承自 AppConfig
struct AppListConfig : public AppConfig
{
    std::vector<AppListForOneConfig> Apps; // 存储应用程序的列表

    // 根据 ID 获取特定的应用配置
    AppListForOneConfig* GetOne(int id)
    {
        for (decltype(Apps.size()) index = 0; index < Apps.size(); index++)
        {
            if (Apps[index].Id == id)
                return &Apps[index]; // 找到匹配的 ID，返回该配置
        }

        return nullptr; // 如果未找到，返回 nullptr
    }
};

// 登录配置，继承自 AppListConfig
struct LoginConfig : public AppListConfig
{
    std::string UrlLogin;   // 登录 URL
    std::string UrlMethod;  // 登录方法
};

// 游戏配置，继承自 AppListConfig
struct GameConfig : public AppListConfig
{

};

// 空间配置，继承自 AppListConfig
struct SpaceConfig : public AppListConfig
{

};

// 数据库配置结构体
struct DBConfig
{
    std::string DBType;       // 数据库类型
    std::string Ip{ "127.0.0.1" }; // 默认数据库 IP 地址
    int Port{ 3306 };         // 默认数据库端口
    std::string User{ "" };   // 数据库用户名
    std::string Password{ "" }; // 数据库密码
    std::string DatabaseName{ "" }; // 数据库名
    std::string CharacterSet{ "" };  // 字符集
    std::string Collation{ "" };      // 排序规则
};

// 数据库管理配置，继承自 CommonConfig
struct DBMgrConfig : public CommonConfig
{
    static std::string DBTypeMysql; // MySQL 数据库类型
    static std::string DBTypeRedis;  // Redis 数据库类型

    std::vector<DBConfig> DBs; // 存储数据库配置列表

    // 根据数据库类型获取特定的数据库配置
    DBConfig* GetDBConfig(const std::string dbType)
    {
        for (decltype(DBs.size()) index = 0; index < DBs.size(); index++)
        {
            if (DBs[index].DBType != dbType)
                continue; // 如果类型不匹配，继续

            return &DBs[index]; // 找到匹配的类型，返回该配置
        }

        return nullptr; // 如果未找到，返回 nullptr
    }
};

// 机器人配置，继承自 AppConfig
struct RobotConfig : public AppConfig
{

};

// YAML 处理类，继承自 Component，并实现 IAwakeSystem 接口
class Yaml : public Component<Yaml>, public IAwakeSystem<>
{
public:
    // 初始化方法
    void Awake() override;
    // 清理方法
    void BackToPool() override;

    // 获取指定应用类型的配置
    YamlConfig* GetConfig(APP_TYPE appType);
    // 获取指定应用 ID 的 IP 端点
    CommonConfig* GetIPEndPoint(APP_TYPE appType, int appId = 0);

private:
    // 加载配置
    void LoadConfig(APP_TYPE appType, YAML::Node& config);
    // 加载数据库配置
    DBConfig LoadDbConfig(YAML::Node node) const;
    // 加载应用列表配置
    void LoadAppList(AppListConfig* pConfig, YAML::Node node) const;

private:
    std::map<APP_TYPE, YamlConfig*> _configs; // 存储不同应用类型的配置映射
};
