#ifndef _GAME_SERVICE_H_
#define _GAME_SERVICE_H_

#include "net/socket_listener.h"

#include "examples/server_proto/echo.pb.h"

using namespace Storm;
using namespace Common;

class GameService : public SocketListener {
public:
	virtual void doClose(NetPacket::ptr pack);
	virtual int doRpcRequest(NetPacket::ptr pack, const RpcRequest& req, RpcResponse& resp);

	virtual int Echo(NetPacket::ptr pack, const EchoRequest& request, EchoResponse& resp);
};

#endif
