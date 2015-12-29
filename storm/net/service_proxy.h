#ifndef _SERVICE_PROXY_H_
#define _SERVICE_PROXY_H_

#include <atomic>

#include "common_header.h"
#include "socket_define.h"
#include "request.h"

#include "util/util_thread.h"
#include "util/util_timelist.h"

namespace Storm {

class SocketConnector;

class ServiceProxyCallBack {
public:
	typedef std::shared_ptr<ServiceProxyCallBack> ptr;
	virtual void dispatch(ReqMessage* req) = 0;
};
typedef std::shared_ptr<ServiceProxyCallBack> ServiceProxyCallBackPtr;

struct RecvPacketHandler {
	Notifier m_notifier;
};

class ServiceProxy : public SocketProxy {
public:
	typedef std::shared_ptr<ServiceProxy> ptr;
	ServiceProxy(SocketConnector* connector);
	bool parseFromString(const string& config);
	virtual ~ServiceProxy(){}

	virtual void onConnect(uint32_t id) {
		LOG("onConnect! %d\n", id);
	}
	virtual void onData(uint32_t id, IOBuffer::ptr buffer);
	virtual void onClose(uint32_t id, uint32_t closeType);
	virtual void doRequest(RecvPacket::ptr pack);
	virtual void doClose(RecvPacket::ptr pack);

	void terminate();

	void doInvoke(ReqMessage* req);
	void finishInvoke(ReqMessage* req);

	void doTimeOut();
	void doTimeClose(uint32_t id);

	uint32_t selectEndPoint(uint32_t requestId);
	void updateEndPoints();

private:
	void setReqMessage(uint32_t requestId, ReqMessage* mess);
	ReqMessage* getAndDelReqMessage(uint32_t requestId);

	void setEndPoints(vector<uint32_t>& eps);
	void delEndPoint(uint32_t id);

protected:
	SocketConnector* m_connector;
	bool m_needLocator;
	string m_objName;

	ProtocolType	m_protocol;

	Mutex m_epMutex;
	vector<uint32_t> m_endPoints;

	//key: service name, ip, port
	map<string, uint32_t> m_clients;

	//发出去的消息
	map<uint32_t, ReqMessage*> m_reqMessages;
	Mutex m_messMutex;
	atomic_uint m_sequeue;

	TimeList<uint32_t, uint32_t> m_timeout;
};

ReqMessage* newRequest(InvokeType type, ServiceProxyCallBackPtr cb = ServiceProxyCallBackPtr());
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
			ret = RespStatus_CoderError;
			break;
		}

	} while (0);
	return ret;
}

}

#endif
