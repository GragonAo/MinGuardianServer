#pragma once

#include "app_type.h"
#include "component.h"
#include "system.h"

#include <log4cplus/logger.h> // 引入 log4cplus 日志库

// Log4 类用于日志记录，继承自 Component 和 IAwakeSystem 接口
class Log4 : public Component<Log4>, public IAwakeSystem<APP_TYPE>
{
public:
    // 在系统启动时调用，传入应用类型
    void Awake(APP_TYPE appType) override;

    // 返回对象到池中时调用
    void BackToPool() override;

protected:
    // 记录调试信息
    void DebugInfo(log4cplus::Logger logger) const;

private:
    // 应用类型，默认值为 APP_None
    APP_TYPE _appType{ APP_TYPE::APP_None };
};
