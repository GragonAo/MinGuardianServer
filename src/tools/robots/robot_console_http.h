#pragma once
#include "libserver/console.h"

class RobotConsoleHttp :public ConsoleCmd
{
public:
	void RegisterHandler() override;
	void HandleHelp() override;

private:	
	void HandleRequestCheck(std::vector<std::string>& params);
	void HandleRequestRegister(std::vector<std::string>& params);
};
