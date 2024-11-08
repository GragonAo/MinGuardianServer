#include "yaml.h"
#include "res_path.h"
#include "app_type.h"
#include "util_string.h"
#include "entity.h"
#include "entity_system.h"
#include "log4_help.h"
#include "component_help.h"

// 数据库管理配置的数据库类型
std::string DBMgrConfig::DBTypeMysql{ "mysql" };
std::string DBMgrConfig::DBTypeRedis{ "redis" };

// Yaml 类的初始化方法
void Yaml::Awake()
{
    auto pResPath = ComponentHelp::GetResPath();
    if (pResPath == nullptr)
    {
        std::cout << "yaml awake failed. can't get ResPath." << std::endl;
        return; // 如果无法获取资源路径，则返回
    }

    const std::string path = pResPath->FindResPath("/engine.yaml"); // 获取 YAML 配置文件的路径
    YAML::Node config = YAML::LoadFile(path); // 加载 YAML 配置文件

    // 加载不同类型的配置
    LoadConfig(APP_TYPE::APP_ALLINONE, config);
    LoadConfig(APP_TYPE::APP_LOGIN, config);
    LoadConfig(APP_TYPE::APP_GAME, config);
    LoadConfig(APP_TYPE::APP_SPACE, config);
    LoadConfig(APP_TYPE::APP_ROBOT, config);
    LoadConfig(APP_TYPE::APP_DB_MGR, config);
    LoadConfig(APP_TYPE::APP_APPMGR, config);

    LOG_DEBUG("Yaml awake is Ok."); // 初始化成功的日志
}

// 清理方法，释放配置资源
void Yaml::BackToPool()
{
    for (auto pObj : _configs)
    {
        delete pObj.second; // 释放配置对象
    }
    _configs.clear(); // 清空配置映射
}

// 根据应用类型获取配置
YamlConfig* Yaml::GetConfig(const APP_TYPE appType)
{
    if (_configs.find(appType) != _configs.end())
    {
        return _configs[appType]; // 如果存在，返回对应的配置
    }

    std::cout << "load config failed. appType:" << GetAppName(appType) << std::endl; // 日志记录失败信息
    return nullptr; // 如果不存在，返回 nullptr
}

// 加载配置的方法
void Yaml::LoadConfig(const APP_TYPE appType, YAML::Node& config)
{
    std::string appTypeName = GetAppName(appType); // 获取应用类型名称
    YAML::Node node = config[appTypeName]; // 获取对应的配置节点
    if (node.IsNull())
    {
        std::cout << "load config failed." << appTypeName.c_str() << std::endl; // 日志记录失败信息
        return;
    }

    YamlConfig* pYamlConfig; // 声明配置指针

    // 根据应用类型创建对应的配置对象
    switch (appType)
    {
    case APP_SPACE:
    {
        auto pConfig = new SpaceConfig();
        auto pAppListConfig = dynamic_cast<AppListConfig*>(pConfig); // 动态转换为 AppListConfig
        LoadAppList(pAppListConfig, node); // 加载应用列表
        pYamlConfig = pConfig; // 指针指向配置对象
        break;
    }
    case APP_GAME:
    {
        auto pConfig = new GameConfig();
        auto pAppListConfig = dynamic_cast<AppListConfig*>(pConfig);
        LoadAppList(pAppListConfig, node);
        pYamlConfig = pConfig;
        break;
    }
    case APP_LOGIN:
    {
        auto pConfig = new LoginConfig();
        pConfig->UrlLogin = node["url_login"].as<std::string>(); // 从 YAML 节点读取登录 URL
        pConfig->UrlRegister = node["url_register"].as<std::string>(); // 从 YAML 节点读取登录 URL
        pConfig->UrlMethod = node["url_method"].as<std::string>(); // 从 YAML 节点读取登录方法
        auto pAppListConfig = dynamic_cast<AppListConfig*>(pConfig);
        LoadAppList(pAppListConfig, node);
        pYamlConfig = pConfig;
        break;
    }
    case APP_DB_MGR:
    {
        auto pConfig = new DBMgrConfig();
        YAML::Node node_dbs = node["dbs"]; // 获取数据库配置节点
        const size_t size = node_dbs.size(); // 数据库配置数量
        for (size_t i = 0; i < size; i++)
        {
            DBConfig one = LoadDbConfig(node_dbs[i]); // 加载单个数据库配置
            pConfig->DBs.push_back(one); // 将配置添加到列表
        }
        pYamlConfig = pConfig;
        break;
    }
    case APP_ROBOT:
    {
        auto pConfig = new RobotConfig();
        pYamlConfig = pConfig;
        break;
    }
    default:
    {
        pYamlConfig = new CommonConfig(); // 默认配置
        break;
    }
    }

    // 设置公共配置的 IP 和端口
    auto pCommon = dynamic_cast<CommonConfig*>(pYamlConfig);
    if (pCommon != nullptr)
    {
        pCommon->Ip = node["ip"].as<std::string>();
        pCommon->Port = node["port"].as<int>();

        if (node["http_port"])
            pCommon->HttpPort = node["http_port"].as<int>();
    }

    // 设置应用程序配置的线程数量
    const auto pAppConfig = dynamic_cast<AppConfig*>(pYamlConfig);
    if (pAppConfig != nullptr)
    {
        if (node["thread_logic"])
            pAppConfig->LogicThreadNum = node["thread_logic"].as<int>();
        else
            pAppConfig->LogicThreadNum = 0;

        if (node["thread_mysql"])
            pAppConfig->MysqlThreadNum = node["thread_mysql"].as<int>();
        else
            pAppConfig->MysqlThreadNum = 0;

        if (node["thread_listen"])
            pAppConfig->ListenThreadNum = node["thread_listen"].as<int>();
        else
            pAppConfig->ListenThreadNum = 0;

        if (node["thread_connector"])
            pAppConfig->ConnectThreadNum = node["thread_connector"].as<int>();
        else
            pAppConfig->ConnectThreadNum = 0;
    }

    // 将配置添加到映射中
    _configs.insert(std::make_pair(appType, pYamlConfig));
}

// 加载数据库配置的方法
DBConfig Yaml::LoadDbConfig(YAML::Node node) const
{
    DBConfig one; // 创建数据库配置对象
    YAML::Node::iterator iter = node.begin(); // 迭代器遍历节点
    while (iter != node.end())
    {
        const std::string key = iter->first.as<std::string>(); // 获取键名
        if (key == "type")
        {
            one.DBType = iter->second.as<std::string>();
            std::transform(one.DBType.begin(), one.DBType.end(), one.DBType.begin(), ::tolower); // 转换为小写
        }

        // 从节点读取其他数据库配置信息
        else if (key == "ip")
            one.Ip = iter->second.as<std::string>();
        else if (key == "port")
            one.Port = iter->second.as<int>();
        else if (key == "user")
            one.User = iter->second.as<std::string>();
        else if (key == "password")
            one.Password = iter->second.as<std::string>();
        else if (key == "character_set")
            one.CharacterSet = iter->second.as<std::string>();
        else if (key == "collation")
            one.Collation = iter->second.as<std::string>();
        else if (key == "database_name")
            one.DatabaseName = iter->second.as<std::string>();

        ++iter; // 移动到下一个节点
    }

    return one; // 返回加载的数据库配置
}

// 加载应用列表的方法
void Yaml::LoadAppList(AppListConfig* pConfig, YAML::Node node) const
{
    YAML::Node node_apps = node["apps"]; // 获取应用列表节点
    int size = node_apps.size(); // 获取应用数量
    for (int i = 0; i < size; i++)
    {
        AppListForOneConfig one;

        // 从节点读取单个应用的配置
        one.Id = node_apps[i]["id"].as<int>();
        one.Ip = node_apps[i]["ip"].as<std::string>();
        one.Port = node_apps[i]["port"].as<int>();
        pConfig->Apps.push_back(one); // 将配置添加到应用列表
    }
}

// 获取特定应用类型和 ID 的配置
CommonConfig* Yaml::GetIPEndPoint(APP_TYPE appType, int appId)
{
    const auto pConfig = GetConfig(appType); // 获取应用配置
    const auto pListCfg = dynamic_cast<AppListConfig*>(pConfig);
    if (pListCfg != nullptr)
    {
        auto pOneCfg = pListCfg->GetOne(appId); // 获取特定应用配置
        if (pOneCfg == nullptr)
        {
            LOG_ERROR("can't find appType yaml's config. appType:" << GetAppName(appType) << " id:" << appId);
            return nullptr; // 日志记录未找到配置
        }

        return pOneCfg; // 返回找到的配置
    }
    auto pAppConfig = dynamic_cast<CommonConfig*>(pConfig);
    if (pAppConfig == nullptr)
    {
        LOG_ERROR("can't find appType yaml's config. appType:" << GetAppName(appType));
        return nullptr; // 日志记录未找到配置
    }

    return pAppConfig; // 返回找到的公共配置
}
