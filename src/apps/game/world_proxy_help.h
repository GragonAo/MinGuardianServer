#pragma once
#include "libserver/common.h"

class Player;

class WorldProxyHelp
{
public:
    static void Teleport(Player* pPlayer, uint64 lastWorldSn, uint64 targetWorldSn);
};

/*
WorldProxyHelp 类为游戏中的世界传送功能提供了一个接口，
使得其他类可以通过调用 Teleport 方法来实现玩家的世界切换。
这样的设计将传送逻辑封装在一个类中，有助于维护代码的清晰和模块化。
*/