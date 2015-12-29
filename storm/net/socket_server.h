#ifndef _STORM_SOCKET_SERVER_H_
#define _STORM_SOCKET_SERVER_H_

#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/select.h>
#include <stdio.h>
#include <thread>

#include <map>

#include "common_header.h"

#include "socket_poller.h"
#include "socket_listener.h"
#include "socket_util.h"
#include "util/object_pool.h"
#include "util/util_thread.h"
#include "app_config.h"

#include "socket_define.h"

#define MAX_SOCKET (1 << 16)
#define MAX_EVENT 1024
#define MAX_INFO 128

namespace Storm {

class SocketServer {
public:
    SocketServer();
    ~SocketServer();

	void poll();

	void send(int id, IOBuffer::ptr buffer);
	void close(int id, uint32_t closeType = CloseType_Server);
	void terminate();
	bool isTerminate() { return m_exit; }
	void doTimer();

    void show();

	template<typename T>
	bool addListener(const ServiceConfig& cfg);

	void start();
private:
    int reserveId();
	void exit();
    void forceClose(Socket* pSocket, int closeType = CloseType_Server);
	inline void pushCmd(const SocketCmd& cmd);

	Socket* getNewSocket();
	Socket* getSocket(int id);

	// 事件处理
	void handleAccept(Socket* s);
	void handleRead(Socket* s);
	void handleWrite(Socket* s);

	// 请求处理
	void handleCmd();
	void sendSocket(int id, IOBuffer::ptr buffer);
	void closeSocket(int id, uint32_t closeType);

private:
	bool m_exit;
    int m_allocId;
    SocketPoller	m_poll;

	Socket* m_socket;
	LockQueue<SocketCmd> m_queue;

    Socket 			m_slot[MAX_SOCKET];
    SocketEvent 	m_event[MAX_EVENT];

	ObjectPool<IOBuffer> m_bufferPool;

	map<string, SocketHandler::ptr>		m_handlers;

    char m_buffer[MAX_INFO];
	std::thread m_netThread;

	uint32_t m_heatBeatTime;
	uint32_t m_heatBeatInterval;
};

template<typename T>
bool SocketServer::addListener(const ServiceConfig& cfg) {
	const string& listenerName = cfg.serviceName;

	map<string, SocketHandler::ptr>::iterator it = m_handlers.find(listenerName);
	if (it != m_handlers.end()) {
		LOG("duplicate listener listenerName: %s\n", listenerName.c_str());
		return false;
	}
	SocketHandler::ptr handler(new SocketHandler);
	if (!handler->setListener<T>(this, cfg)) {
		LOG("setListener error\n");
		return false;
	}
	m_handlers.insert(make_pair(listenerName, handler));

	//监听
	int fd = socketListen(cfg.host.c_str(), cfg.port, 1024);
	if (fd < 0) {
		LOG("error when bind %s\n", listenerName.c_str());
		return false;
	}
	Socket* s = getNewSocket();
	if (s == NULL) {
		LOG("error when getSocket %s\n", listenerName.c_str());
		return false;
	}

	s->fd = fd;
	s->type = Socket_Type_Listen;
	s->status = Socket_Status_Listen;
	s->handler = handler.get();
	m_poll.addToRead(fd, s);

	LOG("start %s, host: %s, port: %d\n", listenerName.c_str(), cfg.host.c_str(), cfg.port);
	return true;
}

}

#endif
