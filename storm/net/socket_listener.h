#ifndef _STORM_SOCKET_LISTENER_H_
#define _STORM_SOCKET_LISTENER_H_

#include "util/util_thread.h"

#include "request.h"
#include "socket_define.h"

#include "proto/rpc_proto.pb.h"

namespace Storm {

class SocketServer;
class SocketHandle;

enum NetPacket_Type {
	Net_Close,
	Net_Packet,
};

struct NetPacket {
	typedef boost::shared_ptr<NetPacket> ptr;
	int type;
	int id;
	int fd;
	int port;
	int closeType;
	string ip;
	string buffer;
};

class SocketListener {
	friend class SocketHandle;
	friend class SocketServer;
public:
	typedef boost::shared_ptr<SocketListener> ptr;

	SocketListener();
	virtual ~SocketListener(){}

	virtual void doClose(NetPacket::ptr pack) {LOG("doClose\n");}

	//自定义协议重载doRequest
	virtual void doRequest(NetPacket::ptr pack);

	//RPC协议重载doRpcRequest
	virtual int doRpcRequest(NetPacket::ptr pack, const RpcRequest& req, RpcResponse& resp) {
		return 0;
	}

	//网络线程调用
	void onAccept(int id, int fd, const string& ip, int port);
	void onClose(int id, int fd, const string& ip, int port, int closeType);
	void onData(int id, int fd, const string& ip, int port, IOBuffer::ptr buffer);

	void setProtocol(ProtocolType protocol) {
		m_protocol = protocol;
	}

	bool checkIpValid(const string& sHost);

	void send(int id, const string& str);
	void send(int id, IOBuffer::ptr buffer);

	void process();

protected:
	string m_listenerName;
	string m_host;
	int	   m_port;
	bool m_isRpc;
	ProtocolType	m_protocol;
	SocketHandle* m_handle;
	SocketServer* m_sockServer;
	LockQueue<NetPacket::ptr> m_msg;
};

class SocketHandle {
	friend class SocketServer;
public:
	typedef boost::shared_ptr<SocketHandle> ptr;

	SocketHandle():m_term(false), m_threadNum(1) {}
	virtual ~SocketHandle() {}

	void start();
	void loop();
	void terminate();

	bool addListener(const string& listenerName, SocketListener::ptr listener);
	void setThreadNum(uint32_t threadNum);

	bool isAllEmpty();
	void wakeup();
	void wakeupAll();

private:
	static const uint32_t kMaxThreadNum = 100;

	bool   m_term;
	uint32_t   m_threadNum;
	string m_handleName;
	vector<SocketListener::ptr> m_listeners;
	Notifier m_notifier;
	vector<Thread::ptr> m_threads;
};

}

#endif
