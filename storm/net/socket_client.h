#ifndef _SOCKET_CLIENT_H_
#define _SOCKET_CLIENT_H_

#include <map>
#include <atomic>

#include "common_header.h"
#include "socket_define.h"
#include "util/util_thread.h"
#include "request.h"

namespace Storm {
class SocketConnector;
class ServiceProxy;

enum RecvType {
	RecvType_Close = 0,
	RecvType_Packet = 1,
};

struct RecvPacket {
	typedef std::shared_ptr<RecvPacket> ptr;
	int type;
	int closeType;
	string buffer;
};

class SocketClient {
public:
	friend class SocketConnector;
	typedef std::shared_ptr<SocketClient> ptr;

	SocketClient();

	void sendPacket(ReqMessage* req);

	//网络线程调用
	void onConnect(int id) {
		LOG("onConnect!\n");
	}

	void onData(IOBuffer::ptr buffer);
	void onClose(uint32_t closeType);

	void process();

	void doClose(RecvPacket::ptr pack);
	void doRequest(RecvPacket::ptr pack);

	void setProtocol(ProtocolType protocol) {
		m_protocol = protocol;
	}

private:
	void setReqMessage(ReqMessage* mess);
	ReqMessage* getReqMessage(uint32_t requestId);
	ReqMessage* getAndDelReqMessage(uint32_t requestId);
	void delReqMessage(uint32_t requestId);

private:
	string m_serviceName;
	string m_host;
	int	   m_port;
	int    m_id;
	ProtocolType	m_protocol;
	SocketConnector* m_connector;
	LockQueue<RecvPacket::ptr> m_packets;

	map<uint32_t, ReqMessage*> m_reqMessages;
	Mutex m_mutex;
	atomic_uint m_sequeue;
};
}

#endif
