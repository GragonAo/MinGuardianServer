// #include "space_collection.h"
// #include "space_channel_component.h"
// #include "libserver/common.h"
// #include "libserver/log4_help.h"
// #include "libserver/message_system.h"
// #include "libserver/protobuf/proto_id.pb.h"
// #include <utility>
// void SpaceCollection::Awake() {
//   // 绑定空间服务器连接成功回调
//   auto messageSystem = _pSystemManager->GetMessageSystem();
//   messageSystem->RegisterFunction(
//       this, Proto::MsgId::MI_NetworkConnect,
//       BindFunP1(this, &SpaceCollection::HandleSpaceServerConnect));

//   messageSystem->RegisterFunction(
//       this, Proto::MsgId::MI_NetworkDisconnect,
//       BindFunP1(this, &SpaceCollection::HandleSpaceServerDisconnect));
// }

// void SpaceCollection::BackToPool() { _networkChannel.clear(); }

// void SpaceCollection::HandleSpaceServerConnect(Packet *pPacket) {
//   auto key = pPacket->GetObjectKey();
//   if (key.KeyType != ObjectKeyType::App)
//     return; // 如果是APP应用，说明是空间层服务器

//   auto serverName = key.KeyValue.KeyStr;
//     auto iter = _networkChannel.find(serverName);
//     if(iter == _networkChannel.end()){
//         _networkChannel.insert(std::make_pair(serverName, std::list<SpaceChannelComponent*>()));
//     }
//     auto spaceList = _networkChannel[serverName];
//     auto space = _pSystemManager->GetEntitySystem()->AddComponent<SpaceChannelComponent>(serverName,pPacket->GetSocketKey().Socket);
//     spaceList.push_back(space);
// }

// void SpaceCollection::HandleSpaceServerDisconnect(Packet *pPacket) {
//   auto key = pPacket->GetObjectKey();
//   if (key.KeyType != ObjectKeyType::App)
//     return; // 如果是APP应用，说明是空间层服务器
//   auto serverName = key.KeyValue.KeyStr;
//   auto iter = _networkChannel.find(serverName);
//   if (iter == _networkChannel.end()) {
//     LOG_ERROR(
//         "Offline space servers do not exist in the collection. serverName: "
//         << serverName);
//     return;
//   }
//   auto sock = pPacket->GetSocketKey().Socket;
//   auto spaceList = iter->second;
//   SpaceChannelComponent *space = nullptr;
//   for (auto one : spaceList) {
//     if (one->GetSocket() == sock) {
//       space = one;
//       break;
//     }
//   }
//   spaceList.remove(space);
// }

// SpaceChannelComponent *
// SpaceCollection::GetSpaceComponent(std::string serverName) {
//   const auto iter = _networkChannel.find(serverName);
//   if (iter == _networkChannel.end()) {
//     LOG_WARN("The server is not connected to this server. serverName: "
//              << serverName);
//     return nullptr;
//   }
//   const auto spaceList = iter->second;
//   SpaceChannelComponent *space = nullptr;
//   int maxv = 0x3fffff;
//   for (auto one : spaceList) {
//     if (maxv > one->GetLoadBalancing()) {
//       space = one;
//       maxv = one->GetLoadBalancing();
//     }
//   }
//   return space;
// }