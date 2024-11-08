#include "robot_console_http.h"
#include "libserver/thread_mgr.h"
#include "test_http_register.h"
#include "test_http_login.h"

void RobotConsoleHttp::RegisterHandler()
{
	OnRegisterHandler("-check", BindFunP1(this, &RobotConsoleHttp::HandleRequestCheck));
	OnRegisterHandler("-register", BindFunP1(this, &RobotConsoleHttp::HandleRequestRegister));
}

void RobotConsoleHttp::HandleHelp()
{
	std::cout << "\t-check test password.\t\tcheck account." << std::endl;
	std::cout << "\t-register test password.\t\tcheck account." << std::endl;
}

void RobotConsoleHttp::HandleRequestCheck(std::vector<std::string>& params)
{
    if (!CheckParamCnt(params, 2))
        return;
    
    ThreadMgr::GetInstance()->CreateComponent<TestHttpLogin>(params[0], params[1]);
}

void RobotConsoleHttp::HandleRequestRegister(std::vector<std::string>& params)
{
    if (!CheckParamCnt(params, 2))
        return;
    
    ThreadMgr::GetInstance()->CreateComponent<TestHttpRegister>(params[0], params[1]);
}