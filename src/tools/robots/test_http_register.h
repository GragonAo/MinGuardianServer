#pragma once
#include "libserver/entity.h"
#include "libserver/system.h"
#include "libserver/network.h"

class TestHttpRegister :public NetIdentify, public Entity<TestHttpRegister>, public IAwakeSystem<std::string, std::string>
{
public:
    void Awake(std::string account, std::string password) override;
    void BackToPool() override;

protected:
    TestHttpRegister* GetTestHttpRegister(NetIdentify* pNetIdentify);

    void HandleNetworkDisconnect(TestHttpRegister* pObj, Packet* pPacket);
    void HandleNetworkConnected(TestHttpRegister* pObj, Packet* pPacket);
    void HandleHttpOuterResponse(TestHttpRegister* pObj, Packet* pPacket);

private:
    std::string _account{ "" };
    std::string _password{ "" };
    std::string _ip{ "" };
    int _port{ 0 };
    std::string _mothed{ "" };
    std::string _path{""};
};

