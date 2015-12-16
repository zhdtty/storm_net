#ifndef _SERVICE_PROXY_H_
#define _SERVICE_PROXY_H_

#include "common_header.h"
#include "socket_define.h"
#include "socket_client.h"
#include "request.h"

#include "util/util_thread.h"

namespace Storm {

class SocketConnector;

class ServiceProxyCallBack {
public:
	typedef std::shared_ptr<ServiceProxyCallBack> ptr;
	virtual void dispatch(ReqMessage* req) = 0;
};
typedef std::shared_ptr<ServiceProxyCallBack> ServiceProxyCallBackPtr;

struct RecvPacketHandler {
	void waitResponse();
	void signal();
	Notifier m_notifier;
};

class ServiceProxy {
public:
	typedef std::shared_ptr<ServiceProxy> ptr;
	ServiceProxy(SocketConnector* connector);
	bool parseFromString(const string& config);
	virtual ~ServiceProxy(){}

	ReqMessage* newRequest(InvokeType type, ServiceProxyCallBackPtr cb = ServiceProxyCallBackPtr());

	void doInvoke(ReqMessage* req);
	void finishInvoke(ReqMessage* req);

protected:
	SocketConnector* m_connector;
	bool m_needLocator;
	string m_objName;
	vector<SocketClient*> m_clients;
};

void delRequest(ReqMessage* message);

template <typename T>
inline int decodeResponse(ReqMessage* message, T& response) {
	int ret = 0;
	do {
		if (message->status != 0) {
			ret = message->status;
			break;
		}
		RpcResponse* resp = message->resp;
		if (resp->ret() != 0) {
			ret = resp->ret();
			break;
		}
		//LOG("len %lu\n", resp->response().size());
		if (!response.ParseFromString(resp->response())) {
			ret = RespStatus_Coder;
			break;
		}

	} while (0);
	return ret;
}

}

#endif
