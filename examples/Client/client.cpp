#include "client.h"

#include <stdio.h>
#include "util/util_time.h"

#include "log.h"

#include "GameService.h"

Client g_server;

GameServiceProxy* g_prx = NULL;

bool Client::initialize() {
	m_count = 0;
	g_prx = m_connector->stringToPrx<GameServiceProxy>("Server@tcp -h 127.0.0.1 -p 1234");
												 //:tcp -h 127.0.0.1 -p 10001");

	return true;
}

void Client::destroy() {

}

class GameServiceProxyCB : public GameServiceProxyCallBack {
public:
	void callback_Echo(int ret, const EchoResponse& response) {
//		cout << response.msg() << endl;
//	LOG("ret %d\n", ret);
		//LOG("msg len %lu\n", response.msg().size());
	}
};

void Client::loop() {
	EchoRequest request;
	request.set_msg("带着相机去旅行");

	//同步调用
	EchoResponse response;
//	int ret = g_prx->Echo(request, response);
	//cout << response.msg() << endl;
//	LOG("ret %d\n", ret);
//	LOG("msg len %lu\n", response.msg().size());

	//异步调用
	ServiceProxyCallBackPtr cb(new GameServiceProxyCB);
	g_prx->async_Echo(cb, request);

	//单向调用
	g_prx->async_Echo(NULL, request);
//	sleep(1);
}

int main(int argc, char** argv) {
	setDefaultConfigFile("client.conf");
	return g_server.run(argc, argv);
}

