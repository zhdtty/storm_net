#ifndef _GAME_SERVICE_PROXY_H_
#define _GAME_SERVICE_PROXY_H_

#include "net/service_proxy.h"

#include "examples/server_proto/echo.pb.h"

using namespace Storm;
using namespace Common;

class GameServiceProxy : public ServiceProxy {
public:
	GameServiceProxy(SocketConnector* connector):ServiceProxy(connector) {}

	int Echo(const EchoRequest& request, EchoResponse& response);
	void async_Echo(ServiceProxyCallBackPtr cb, const EchoRequest& request);

};

class GameServiceProxyCallBack : public ServiceProxyCallBack {
public:
	virtual void dispatch(ReqMessage* req);

	virtual void callback_Echo(int ret, const EchoResponse& response) {}
};

#endif
