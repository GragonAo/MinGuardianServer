#include "common.h"
#include "log4.h"

#include <iostream>
#include <log4cplus/configurator.h>
#include <log4cplus/spi/loggingevent.h>
#include <log4cplus/logger.h>

#include "util_string.h"
#include "thread_mgr.h"
#include "res_path.h"
#include "app_type.h"
#include "log4_help.h"
#include "component_help.h"

// 系统启动时初始化日志配置
void Log4::Awake(APP_TYPE appType)
{
    _appType = appType; // 保存应用类型
    auto pResPath = ComponentHelp::GetResPath(); // 获取资源路径组件

    // 构建日志配置文件名
    const std::string filename = strutil::format("/log4/log4_%s.properties", GetAppName(_appType));
    std::string filePath = pResPath->FindResPath(filename); // 查找日志配置文件路径
    if (filePath.empty())
    {
        std::cout << " !!!!! log4 properties not found! filename:" << filename.c_str() << std::endl;
        return; // 文件未找到则返回
    }

    // 将文件路径转换为 log4cplus 的格式
    const log4cplus::tstring configFile = LOG4CPLUS_STRING_TO_TSTRING(filePath);

    // 配置日志
    log4cplus::PropertyConfigurator config(configFile);
    config.configure();

    // 输出调试信息
    DebugInfo(log4cplus::Logger::getRoot());
    DebugInfo(log4cplus::Logger::getInstance(LOG4CPLUS_C_STR_TO_TSTRING("GM")));
    LOG_DEBUG("Log4::Initialize is Ok."); // 输出初始化成功的日志
}

// 输出指定 logger 的调试信息
void Log4::DebugInfo(log4cplus::Logger logger) const
{
    // 获取所有附加器
    log4cplus::SharedAppenderPtrList appenderList = logger.getAllAppenders();
    auto iter = appenderList.begin();
    while (iter != appenderList.end())
    {
        log4cplus::Appender* pAppender = iter->get(); // 获取附加器

        // 注释掉的部分可以设置附加器的区域设置，例如中文
        // log4cplus::RollingFileAppender* pFileAppender = static_cast<log4cplus::RollingFileAppender*>(pAppender);
        // if (pFileAppender != nullptr)
        //     pFileAppender->imbue(std::locale("zh_CN"));

        // log4cplus::ConsoleAppender* pConsoleAppender = static_cast<log4cplus::ConsoleAppender*>(pAppender);
        // if (pConsoleAppender != nullptr)
        //     pConsoleAppender->imbue(std::locale("zh_CN"));

        // 输出 logger 名称和附加器名称
        std::cout << "[log4] " << LOG4CPLUS_TSTRING_TO_STRING(logger.getName()) << " appender name:" << LOG4CPLUS_TSTRING_TO_STRING(pAppender->getName()) << std::endl;
        ++iter; // 迭代到下一个附加器
    }
}

// 返回对象到池中，当前没有实现具体逻辑
void Log4::BackToPool()
{
}
