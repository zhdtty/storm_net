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
	void close(int id);
	void terminate();
	bool isTerminate() { return m_exit; }

    void show();

	template<typename T>
	bool addListener(const ServiceConfig& cfg);

	void start();
private:
    int reserveId();
	void exit();
    void forceClose(Socket* pSocket, int closeType = Server_Close);
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
	void closeSocket(int id);

private:
	bool m_exit;
    int m_allocId;
    SocketPoller	m_poll;

	Socket* m_socket;
	LockQueue<SocketCmd> m_queue;

    Socket 			m_slot[MAX_SOCKET];
    SocketEvent 	m_event[MAX_EVENT];

	ObjectPool<IOBuffer> m_bufferPool;

	map<string, SocketHandle::ptr>		m_handles;
	map<string, SocketListener::ptr>	m_listeners;

    char m_buffer[MAX_INFO];
	std::thread m_netThread;
};

template<typename T>
bool SocketServer::addListener(const ServiceConfig& cfg) {
	const string& handleName = cfg.group;
	const string& listenerName = cfg.serviceName;
	map<string, SocketHandle::ptr>::iterator itHandle = m_handles.find(handleName);
	SocketHandle::ptr handle;
	if (itHandle != m_handles.end()) {
		handle = itHandle->second;
	} else {
		handle.reset(new SocketHandle);
		handle->m_handleName = handleName;
		handle->m_threadNum = cfg.threadNum;
		m_handles.insert(make_pair(handleName, handle));
	}

	map<string, SocketListener::ptr>::iterator itListen = m_listeners.find(listenerName);
	if (itListen != m_listeners.end()) {
		LOG("duplicate listener name: %s\n", listenerName.c_str());
		return false;
	}
	SocketListener::ptr listener(new T);
	listener->m_listenerName = cfg.serviceName;
	listener->m_host = cfg.host;
	listener->m_port = cfg.port;
	listener->m_handle = handle.get();
	listener->m_sockServer = this;

	m_listeners.insert(make_pair(listenerName, listener));
	handle->addListener(listenerName, listener);

	//监听
	int fd = socketListen(listener->m_host.c_str(), listener->m_port, 1024);
	if (fd < 0) {
		LOG("error when bind %s\n", listener->m_listenerName.c_str());
		return false;
	}
	Socket* s = getNewSocket();
	if (s == NULL) {
		LOG("error when getSocket %s\n", listener->m_listenerName.c_str());
		return false;
	}

	s->fd = fd;
	s->type = Socket_Type_Listen;
	s->status = Socket_Status_Listen;
	s->listener = listener.get();
	m_poll.addToRead(fd, s);

	LOG("start %s, host: %s, port: %d\n", listenerName.c_str(), cfg.host.c_str(), cfg.port);
	return true;
}

}

#endif
