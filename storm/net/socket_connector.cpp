#include "socket_connector.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include "util/util_string.h"
#include "util/util_time.h"

#define HASH_ID(id) (((unsigned)id) % MAX_CLIENT)

namespace Storm {
SocketConnector::SocketConnector()
	:m_exit(false), m_allocId(-1) {
    if (m_poll.invalid()) {
        LOG("create poll failed\n");
        ::exit(1);
    }

	int fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if (fd < 0) {
		LOG("efd < 0\n");
		::exit(1);
	}

	m_socket = getNewSocket();
	m_socket->fd = fd;
	m_socket->type = Socket_Type_Notify;
	m_socket->status = Socket_Status_Listen;

	m_poll.addEvent(fd, EP_READ, m_socket);
}

void SocketConnector::start() {
	m_netThread.start(std::bind(&SocketConnector::poll, this));

	LOG("socket connector start\n");
	//异步线程启动
	for (uint32_t i = 0; i < 1; ++i) {
		Thread::ptr t(new Thread());
		m_asyncThreads.push_back(t);
		t->start(std::bind(&SocketConnector::loop, this));
	}
}

void SocketConnector::exit() {
	if (m_exit) {
		return ;
	}
	m_exit = true;

    for (int i = 0; i < MAX_CLIENT; ++i) {
		Socket* s = &m_slot[i];
		forceClose(s);
    }

	//异步线程停止
	wakeupAll();
	for (uint32_t i = 0; i < m_asyncThreads.size(); ++i) {
		m_asyncThreads[i]->join();
	}
}

void SocketConnector::loop() {
	while (!m_exit) {
		if (isAllEmpty()) {
			ScopeMutex<Notifier> lock(m_notifier);
			m_notifier.wait();
		}
		for (map<string, SocketClient::ptr>::iterator it =  m_clients.begin(); it != m_clients.end(); ++it) {
			it->second->process();
		}
	}
}

void SocketConnector::wakeup() {
	ScopeMutex<Notifier> lock(m_notifier);
	m_notifier.signal();
}

void SocketConnector::wakeupAll()  {
	ScopeMutex<Notifier> lock(m_notifier);
	m_notifier.broadcast();
}

bool SocketConnector::isAllEmpty() {
	for (map<string, SocketClient::ptr>::iterator it =  m_clients.begin(); it != m_clients.end(); ++it) {
		if (it->second->m_packets.empty() == false) {
			return false;
		}
	}
	return true;
}


inline Socket* SocketConnector::getNewSocket() {
	int id = reserveId();
	return getSocket(id);
}

inline Socket* SocketConnector::getSocket(int id) {
	if (id < 0) {
		return NULL;
	}
	return &m_slot[HASH_ID(id)];
}

int SocketConnector::reserveId() {
	for (int i = 0; i < MAX_CLIENT; ++i) {
		int id = __sync_add_and_fetch(&m_allocId, 1);
		if (id < 0) {
			id = __sync_and_and_fetch(&m_allocId, 0x7fffffff);
		}
		Socket* s = &m_slot[HASH_ID(id)];
		if (s->status == Socket_Status_Idle) {
			if (__sync_bool_compare_and_swap(&s->status, Socket_Status_Idle, Socket_Status_Reserve)) {
				s->id = id;
				s->fd = -1;
				return id;
			} else {
				--i;
			}
		}
	}
    return -1;
}

void SocketConnector::forceClose(Socket* s, int closeType) {
	if (s->status == Socket_Status_Idle || s->status == Socket_Status_Reserve) {
		return;
	}

	//关闭
	m_poll.delEvent(s->fd, EP_READ|EP_WRITE, s);
	::close(s->fd);
	s->status = Socket_Status_Reserve;

	//把读写缓存放回缓存池
	if (s->readBuffer) {
		s->readBuffer->reset();
		m_bufferPool.put(s->readBuffer);
		s->readBuffer.reset();
	}
	for (list<IOBuffer::ptr>::iterator it = s->writeBuffer.begin(); it != s->writeBuffer.end(); ) {
		s->writeBuffer.erase(it++);
	}

	//通知
	if (s->type == Socket_Type_ToOut) {
		s->client->onClose(closeType);
	}
}

void SocketConnector::handleCmd() {
	uint64_t d = 0;
	::read(m_socket->fd, &d, sizeof(d));

	SocketCmd cmd;
	while (m_queue.pop_front(cmd, 0)) {
		switch(cmd.type) {
			case Socket_Cmd_Send:
				sendSocket(cmd.id, cmd.buffer);
				break;
			case Socket_Cmd_Close:
				closeSocket(cmd.id);
				break;
			case Socket_Cmd_Exit:
				exit();
				break;
		}
	}
}

void SocketConnector::sendSocket(int id, IOBuffer::ptr buffer) {
	Socket* s = getSocket(id);
	assert(s != NULL);
	if (s->status == Socket_Status_Reserve) {
		//没有连接，加到buffer list，建立链接
		s->writeBuffer.push_back(buffer);
		connectSocket(s);
		return;
	} else if (s->status == Socket_Status_Connecting) {
		//连接中，加到buffer list
		s->writeBuffer.push_back(buffer);
		return;
	}

	if (s->status != Socket_Status_Connected) {
		return;
	}

	char* sendBuf = buffer->getHead();
	int remainSize = buffer->getSize();
	bool needClose = false;
	if (s->writeBuffer.empty()) {
		while (1) {
			if (remainSize == 0) {
				break;
			}
			int sendSize = ::write(s->fd, sendBuf, remainSize);
			if (sendSize == 0) {
				needClose = true;
				break;
			} else if (sendSize < 0) {
				if (errno == EAGAIN) {	//不能再写了
					m_poll.addEvent(s->fd, EP_WRITE, s);
					break;
				} else if (errno == EINTR) {
					continue;
				} else {
					needClose = true;
					break;
				}
			} else {
				sendBuf += sendSize;
				remainSize -= sendSize;
				buffer->readN(sendSize);
			}
		}
	}
	if (needClose) {
		forceClose(s, CloseType_Server);
	} else if (remainSize != 0) {
		s->writeBuffer.push_back(buffer);
	}
}

void SocketConnector::connectSocket(Socket* s) {
	bool success = false;
	do {
		int fd = ::socket(AF_INET, SOCK_STREAM, 0);
		if (fd < 0) {
			break;
		}
		socketKeepAlive(fd);
		socketNonBlock(fd);

		//LOG("ip %s, port %d\n", s->ip.c_str(), s->port);
		struct sockaddr_in stAddr;
		memset(&stAddr, 0, sizeof(stAddr));
		stAddr.sin_family = AF_INET;
		stAddr.sin_port = htons(s->port);
		inet_aton(s->ip.c_str(), &stAddr.sin_addr);

		int status = ::connect(fd, (struct sockaddr*)&stAddr, sizeof(stAddr));
		if (status != 0 && errno != EINPROGRESS) {
			::close(fd);
			LOG("connect error: %s.\n", strerror(errno));
			break;
		}

		success = true;

		s->fd = fd;

		if (status == 0) {
			//连接成功
			s->status = Socket_Status_Connected;
			s->client->onConnect(s->id);
			m_poll.addEvent(fd, EP_READ, s);
			handleWrite(s);
		} else {
			s->status = Socket_Status_Connecting;
			m_poll.addEvent(fd, EP_WRITE, s);
		}
	} while (0);

	if (!success) {
		LOG("socket-server: connect socket error\n");
		s->status = Socket_Status_Reserve;
		s->client->onClose(CloseType_ConnectFail);
	}
}

void SocketConnector::closeSocket(int id) {
	Socket* s = getSocket(id);
	if (s == NULL || s->status == Socket_Status_Idle) {
		return;
	}
	forceClose(s, CloseType_Client);
}

void SocketConnector::handleConnect(Socket* s)
{
    int iError;
    socklen_t iLen = sizeof(iError);
    int iCode = getsockopt(s->fd, SOL_SOCKET, SO_ERROR, &iError, &iLen);
    if (iCode < 0 || iError) {
		LOG("connect error: %s.\n", strerror(iError));
        forceClose(s, CloseType_ConnectFail);
		return;
    }
	s->status = Socket_Status_Connected;
	m_poll.delEvent(s->fd, EP_WRITE, s);

	s->client->onConnect(s->id);
	m_poll.addEvent(s->fd, EP_READ, s);
	handleWrite(s);
}

void SocketConnector::handleRead(Socket* s) {
	if (s->status != Socket_Status_Connected) {
		return;
	}
	if (!s->readBuffer) {
		s->readBuffer = m_bufferPool.get();
	}
	IOBuffer::ptr buffer = s->readBuffer;

	bool needClose = false;
	while (1) {
		uint32_t maxSize = buffer->getRemainingSize();
		//缓存满了，2倍扩充
		if (maxSize == 0) {
			buffer->doubleCapacity();
		}
		maxSize = buffer->getRemainingSize();
		int readSize = ::read(s->fd, buffer->getTail(), maxSize);
		if (readSize == 0) {
			needClose = true;
			break;
		} else if (readSize < 0) {
    		if(errno == EAGAIN) { //没数据可以读
    			break;
    		} else if(errno == EINTR) {
    			continue;
    		} else {
				LOG("connection error fd: %d error:%s\n", s->fd, strerror(errno));
				needClose = true;
    			break;
    		}
		} else {
			buffer->writeN(readSize);
		}
	}
	if (needClose) {
		forceClose(s, CloseType_Server);
	} else {
		s->client->onData(buffer);
	}
}

void SocketConnector::handleWrite(Socket* s) {
	if (s->status != Socket_Status_Connected) {
		return;
	}

	char* d = NULL;
	int size = 0;
	int sendSize = 0;
	bool needClose = false;

	while (1) {
		if (s->writeBuffer.empty()) {
			m_poll.delEvent(s->fd, EP_WRITE, s);
			break;
		}
		IOBuffer::ptr buffer = s->writeBuffer.front();
		d = buffer->getHead();
		size = buffer->getSize();

		sendSize = ::write(s->fd, d, size);
		if (sendSize == 0) {
			needClose = true;
			break;
		} else if (sendSize < 0) {
			if (errno == EAGAIN) {	//不能再写了
				break;
			} else if (errno == EINTR) {
				continue;
			} else {
				needClose = true;
				break;
			}
		} else {
			buffer->readN(sendSize);
			if (buffer->getSize() == 0) {
				s->writeBuffer.pop_front();
			}
		}
	}

	if (needClose) {
		forceClose(s, CloseType_Server);
	}
}

void SocketConnector::poll() {
	while (!m_exit) {
		int num = m_poll.wait(m_event, MAX_EVENT);
		if (num <= 0) {
			continue ;
		}
		for (int i = 0; i < num; ++i) {
			SocketEvent* event = &m_event[i];
			Socket* s = (Socket*)(event->pUd);
			switch (s->type) {
				case Socket_Type_Notify:
					handleCmd();
					break;
				case Socket_Type_ToOut:
				{
					if (s->status == Socket_Status_Connecting) {
						handleConnect(s);
					} else {
						if (event->bRead) {
							handleRead(s);
						}
						if (event->bWrite) {
							handleWrite(s);
						}
					}
					break;
				}
				default:
					LOG("invalid sock type %d\n", s->type);
					break;
			}
		}
	}
}

inline void SocketConnector::pushCmd(const SocketCmd& cmd) {
	m_queue.push_back(cmd);
	uint64_t d = 1;
	::write(m_socket->fd, &d, sizeof(d));
}

void SocketConnector::send(int id, IOBuffer::ptr buffer) {
	SocketCmd cmd;
	cmd.type = Socket_Cmd_Send;
	cmd.id = id;
	cmd.buffer = buffer;

	pushCmd(cmd);
}

void SocketConnector::close(int id) {
	SocketCmd cmd;
	cmd.id = id;
	cmd.type = Socket_Cmd_Close;

	pushCmd(cmd);
}

void SocketConnector::terminate() {
	SocketCmd cmd;
	cmd.type = Socket_Cmd_Exit;

	pushCmd(cmd);

	m_netThread.join();
}

SocketClient::ptr SocketConnector::getSocketClient(const string& serviceName, const string& host, uint32_t port) {
	string key = serviceName + "@" + host + ":" + UtilString::tostr(port);

	map<string, SocketClient::ptr>::iterator it =  m_clients.find(key);
	if (it != m_clients.end()) {
		return it->second;
	}
	SocketClient::ptr client(new SocketClient);
	client->m_serviceName = serviceName;
	client->m_host = host;
	client->m_port = port;
	client->m_connector = this;

	Socket* s = getNewSocket();
	if (s == NULL) {
		LOG("error when getSocket %s\n", key.c_str());
		return client;
	}
	client->m_id = s->id;

	s->type = Socket_Type_ToOut;
	s->status = Socket_Status_Reserve;
	s->client = client.get();
	s->ip = host;
	s->port = port;

	m_clients.insert(make_pair(key, client));

	return client;
}

}

