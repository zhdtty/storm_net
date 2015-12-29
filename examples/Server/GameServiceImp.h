#ifndef _GAME_SERVICE_IMP_H_
#define _GAME_SERVICE_IMP_H_

#include "GameService.h"

class GameServiceImp : public GameService {
public:
	virtual void doClose(Connection::ptr pack);

	virtual void Echo(Connection::ptr pack, const EchoRequest& req, EchoResponse& resp);
};

#endif
