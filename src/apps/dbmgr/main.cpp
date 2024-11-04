#include "libserver/common.h"
#include "libserver/app_type.h"
#include "libserver/server_app.h"
#include "libserver/network_listen.h"
#include "libserver/component_help.h"

#include "dbmgr.h"
#include "mysql_table_update.h"
#include "libserver/global.h"

int main(int argc, char* argv[])
{
    // 初始化 MySQL 库，准备与 MySQL 进行交互
    mysql_library_init(0, nullptr, nullptr);

    // 定义当前应用程序的类型为数据库管理器 (DB Manager)
    const APP_TYPE curAppType = APP_TYPE::APP_DB_MGR;
    
    // 创建一个 ServerApp 实例，并根据传入的参数初始化应用
    ServerApp app(curAppType, argc, argv);
    app.Initialize();

    // 获取 MysqlTableUpdate 的实例（单例模式），并调用 Check 函数检查和更新数据库
    auto pTableUpdateObj = MysqlTableUpdate::Instance();
    pTableUpdateObj->Check();
    pTableUpdateObj->DestroyInstance();  // 销毁实例

    // 获取线程管理器实例，初始化数据库管理器相关的组件
    auto pThreadMgr = ThreadMgr::GetInstance();

    InitializeComponentDBMgr(pThreadMgr);  // 初始化数据库管理器组件

    // 获取全局管理实例，并创建网络监听组件，准备侦听网络请求
    const auto pGlobal = Global::GetInstance();
    pThreadMgr->CreateComponent<NetworkListen>(
        ListenThread, false, 
        (int)pGlobal->GetCurAppType(), pGlobal->GetCurAppId()
    );
    
    // 运行应用程序主循环，进入事件处理
    app.Run();

    // 应用程序退出时释放资源
    app.Dispose();

    // 结束 MySQL 库，释放与 MySQL 相关的资源
    mysql_library_end();
    return 0;
}
