#pragma once

#include "libserver/common.h"
#include "libserver/component.h"
#include "libserver/socket_object.h"
#include "libserver/system.h"

class SpaceChannelComponent : public Component<SpaceChannelComponent>,
                              public IAwakeFromPoolSystem<std::string,SOCKET> {
public:
  void Awake(std::string serverName,SOCKET sock) override;
  void BackToPool() override;
  SOCKET GetSocket() const;
  int GetLoadBalancing() const;
  void AddPlayer(uint64 uid);
  bool RemovePlayer(uint64 uid);

private:
  std::set<uint64> _players;
  std::string _serverName{""};
  SOCKET _sock{INVALID_SOCKET};
};