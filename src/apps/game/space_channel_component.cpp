#include "space_channel_component.h"
#include "libserver/common.h"

void SpaceChannelComponent::Awake(std::string serverName, SOCKET sock) {
  _serverName = serverName;
  _sock = sock;
}

void SpaceChannelComponent::BackToPool() { _players.clear(); }

SOCKET SpaceChannelComponent::GetSocket() const { return _sock; }

int SpaceChannelComponent::GetLoadBalancing() const { return _players.size(); }

void SpaceChannelComponent::AddPlayer(uint64 uid) { _players.insert(uid); }

bool SpaceChannelComponent::RemovePlayer(uint64 uid) {
  if (!_players.count(uid))
    return false;
  _players.erase(uid);
  return true;
}