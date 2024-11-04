#pragma once
#include "libserver/component.h"
#include "libserver/system.h"
#include "libserver/network.h"
#include "space_channel_component.h"
class SpaceCollection : public Entity<SpaceCollection>,public IAwakeFromPoolSystem<>{
public:
    void Awake() override;
    void BackToPool() override;
    void HandleSpaceServerConnect(Packet * pPacket);
    void HandleSpaceServerDisconnect(Packet * pPacket);
    SpaceChannelComponent* GetSpaceComponent(std::string serverName);

private:
    std::map<std::string,std::list<SpaceChannelComponent*>> _networkChannel;
};