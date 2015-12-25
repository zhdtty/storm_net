#ifndef _GAME_SERVICE_IMP_H_
#define _GAME_SERVICE_IMP_H_

#include "GameService.h"

class GameServiceImp : public GameService {
public:
	virtual void doClose(NetPacket::ptr pack);

	virtual void Echo(NetPacket::ptr pack, const EchoRequest& req, EchoResponse& resp);
};

#endif
