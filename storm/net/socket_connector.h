#ifndef _SOCKET_CONNECTOR_H_
#define _SOCKET_CONNECTOR_H_

#include <thread>

#include "common_header.h"

#include "socket_define.h"
#include "socket_client.h"
#include "socket_util.h"
#include "socket_poller.h"
#include "service_proxy.h"

#include "util/object_pool.h"
#include "util/util_thread.h"
#include "util/util_timelist.h"

#include <map>

#define MAX_CLIENT 1024
#define MAX_EVENT  1024

namespace Storm {

class SocketConnector {
public:
	SocketConnector();
	~SocketConnector();

	void poll();
	void start();

	SocketClient::ptr getSocketClient(const string& serviceName, const string& host, uint32_t port);

	void send(int id, IOBuffer::ptr buffer);
	void close(int id, uint32_t closeType = CloseType_Client);
	void terminate();
	bool isTerminate() { return m_exit; }

	template <typename T>
	T* stringToPrx(const string& serviceName) {
		//TODO 加锁
		map<string, ServiceProxy*>::iterator it = m_proxys.find(serviceName);
		if (it != m_proxys.end()) {
			return dynamic_cast<T*>(it->second);
		}
		T* prx = new T(this);
		if (prx->parseFromString(serviceName) == false) {
			LOG("error! string: %s\n", serviceName.c_str());
			return NULL;
		}
		m_proxys.insert(std::make_pair(serviceName, prx));
		return prx;
	}

	void loop();
	void wakeup();
	void wakeupAll();
	bool isAllEmpty();

private:
    int reserveId();
	void exit();
    void forceClose(Socket* pSocket, int closeType = CloseType_Client);

	inline Socket* getNewSocket();
	inline Socket* getSocket(int id);

	inline void pushCmd(const SocketCmd& cmd);

	// 请求处理
	void handleCmd();
	void sendSocket(int id, IOBuffer::ptr buffer);
	void closeSocket(int id, uint32_t closeType);
    void connectSocket(Socket* s);

	// 事件处理
	void handleRead(Socket* s);
	void handleWrite(Socket* s);
	void handleConnect(Socket* s);

	void setUpTimeOut();
	void doConnectTimeOut(uint32_t id);

private:
	bool m_exit;
    int m_allocId;
    SocketPoller	m_poll;

	Socket* m_socket;
	LockQueue<SocketCmd> m_queue;

    Socket 			m_slot[MAX_CLIENT];
    SocketEvent 	m_event[MAX_EVENT];

	ObjectPool<IOBuffer> m_bufferPool;
	std::thread m_netThread;

	//key: service name, ip, port
	map<string, SocketClient::ptr> m_clients;

	//proxys
	map<string, ServiceProxy*> m_proxys;

	//异步业务线程
	vector<std::thread> m_asyncThreads;
	Notifier m_notifier;

	TimeList<uint32_t, uint32_t> m_connTimeout; //连接超时队列
};

}

#endif
