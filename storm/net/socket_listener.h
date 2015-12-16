#ifndef _STORM_SOCKET_LISTENER_H_
#define _STORM_SOCKET_LISTENER_H_

#include <algorithm>

#include "util/util_thread.h"
#include "util/util_timelist.h"

#include "request.h"
#include "socket_define.h"
#include "app_config.h"

#include "proto/rpc_proto.pb.h"

namespace Storm {

class SocketServer;
class SocketHandler;

enum NetPacket_Type {
	Net_Close,
	Net_Packet,
};

struct NetPacket {
	typedef std::shared_ptr<NetPacket> ptr;
	int type;
	int id;
	int fd;
	int port;
	int closeType;
	string ip;
	string buffer;
};

class SocketListener {
	friend class SocketHandler;
public:
	typedef std::shared_ptr<SocketListener> ptr;

	SocketListener();
	virtual ~SocketListener(){}

	virtual bool initialize() {return true;}

	virtual void doClose(NetPacket::ptr pack) {}

	//自定义协议重载doRequest
	virtual void doRequest(NetPacket::ptr pack);

	//RPC协议重载doRpcRequest
	virtual int doRpcRequest(NetPacket::ptr pack, const RpcRequest& req, RpcResponse& resp) {
		return 0;
	}

	void send(int id, const string& str);
	void send(int id, IOBuffer::ptr buffer);

	void setProtocol(ProtocolType protocol);

	void start();
	void terminate();
	void process();

	void showLen();

protected:
	SocketHandler* m_handler;
	SocketServer* m_sockServer;
	std::thread m_thread;
};

class SocketHandler {
	friend class SocketServer;
	friend class SocketListener;
public:
	typedef std::shared_ptr<SocketHandler> ptr;

	SocketHandler();
	virtual ~SocketHandler() {}

	void start();
	void terminate();

	template <typename T>
	bool setListener(SocketServer* server, const ServiceConfig& config);

	void setProtocol(ProtocolType protocol) {
		m_protocol = protocol;
	}

	//网络线程调用
	void onAccept(int id, int fd, const string& ip, int port);
	void onClose(int id, int fd, const string& ip, int port, int closeType);
	void onData(int id, int fd, const string& ip, int port, IOBuffer::ptr buffer);

	bool checkIpValid(const string& sHost) { return true; }

	void setUpTimeOut();
	void checkTimeOut();
	void doTimeClose(uint32_t id);
	void doEmptyClose(uint32_t id);

private:
	static const uint32_t kMaxThreadNum = 100;

	bool   m_term;
	ServiceConfig m_config;
	SocketServer* m_sockServer;
	ProtocolType	m_protocol;
	vector<SocketListener::ptr> m_listeners;
	LockQueue<NetPacket::ptr> m_packets;
	TimeList<uint32_t, uint32_t> m_conList;  //新连接检测
	TimeList<uint32_t, uint32_t> m_timelist; //超时检测
};

template <typename T>
bool SocketHandler::setListener(SocketServer* server, const ServiceConfig& config) {
	m_config = config;
	m_sockServer = server;
	setUpTimeOut();

	uint32_t threadNum = m_config.threadNum;
	threadNum = min(kMaxThreadNum, max((uint32_t)1, threadNum));

	for (uint32_t i = 0; i < threadNum; ++i) {
		SocketListener::ptr listener(new T());
		listener->m_sockServer = server;
		listener->m_handler = this;
		if (!listener->initialize()) {
			return false;
		}
		m_listeners.push_back(listener);
	}
	return true;
}

}

#endif
