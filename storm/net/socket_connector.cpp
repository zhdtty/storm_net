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

	m_updateProxyTime = 0;
	m_updateInterval = 60;
}

void SocketConnector::start(const ClientConfig& cfg) {
	setUpTimeOut(cfg.connectTimeOut);
	m_netThread = std::thread(&SocketConnector::poll, this);

	//异步线程启动
	for (uint32_t i = 0; i < cfg.asyncThreadNum; ++i) {
		m_asyncThreads.push_back(std::thread(&SocketConnector::loop, this));
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
	m_packets.wakeup();
	for (uint32_t i = 0; i < m_asyncThreads.size(); ++i) {
		m_asyncThreads[i].join();
	}

	//proxy的终止
	for (map<string, ServiceProxy*>::iterator it = m_proxys.begin(); it != m_proxys.end(); ++it) {
		it->second->terminate();
	}
}

void SocketConnector::loop() {
	RecvPacket::ptr pack;
	while (!m_exit) {
		if (m_packets.pop_front(pack, -1)) {
			if (pack->type == Net_Close) {
				pack->proxy->doClose(pack);
			} else {
				pack->proxy->doRequest(pack);
			}
		}
	}
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
		s->proxy->onClose(s->id, closeType);
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
				closeSocket(cmd.id, cmd.closeType);
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
	s->writeBuffer.push_back(buffer);

	if (s->status == Socket_Status_Reserve) {
		connectSocket(s);
		return;
	}
	if (s->status == Socket_Status_Connected) {
		handleWrite(s);
	}
}

void SocketConnector::connectSocket(Socket* s) {
	uint64_t nowMs = UtilTime::getNowMS();
	m_connTimeout.add(s->id, nowMs);
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
			s->proxy->onConnect(s->id);
			m_poll.addEvent(fd, EP_READ, s);
			handleWrite(s);
			m_connTimeout.del(s->id);
		} else {
			s->status = Socket_Status_Connecting;
			m_poll.addEvent(fd, EP_WRITE, s);
		}
	} while (0);

	if (!success) {
		LOG("socket-server: connect socket error\n");
		s->status = Socket_Status_Reserve;
		s->proxy->onClose(s->id, CloseType_ConnectFail);
		m_connTimeout.del(s->id);
	}
}

void SocketConnector::closeSocket(int id, uint32_t closeType) {
	Socket* s = getSocket(id);
	if (s == NULL || s->status == Socket_Status_Idle) {
		return;
	}
	forceClose(s, closeType);
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

	s->proxy->onConnect(s->id);
	m_poll.addEvent(s->fd, EP_READ, s);
	handleWrite(s);
	m_connTimeout.del(s->id);
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
		s->proxy->onData(s->id, buffer);
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
		int num = m_poll.wait(m_event, MAX_EVENT, 100);
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
		doTimer();
	}
}

void SocketConnector::doTimer() {
	uint64_t nowMs = UtilTime::getNowMS();
	uint32_t now = UtilTime::getNow();

	//连接超时
	m_connTimeout.timeout(nowMs);

	//各proxy的请求超时
	ScopeMutex<Mutex> lock(m_mutex);
	for (map<string, ServiceProxy*>::iterator it = m_proxys.begin(); it != m_proxys.end(); ++it) {
		it->second->doTimeOut();
	}
	lock.unlock();

	if (now > m_updateProxyTime) {
		updateProxyEndPoints();
		m_updateProxyTime = now + m_updateInterval;
	}
}

void SocketConnector::updateProxyEndPoints() {
	ScopeMutex<Mutex> lock(m_mutex);
	for (map<string, ServiceProxy*>::iterator it = m_proxys.begin(); it != m_proxys.end(); ++it) {
		it->second->updateEndPoints();
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

void SocketConnector::close(int id, uint32_t closeType) {
	SocketCmd cmd;
	cmd.id = id;
	cmd.type = Socket_Cmd_Close;
	cmd.closeType = closeType;

	pushCmd(cmd);
}

void SocketConnector::terminate() {
	SocketCmd cmd;
	cmd.type = Socket_Cmd_Exit;

	pushCmd(cmd);

	m_netThread.join();
}

uint32_t SocketConnector::getNewClientSocket(SocketProxy* proxy, const string& host, uint32_t port) {
	Socket* s = getNewSocket();
	if (s == NULL) {
		LOG("error when getSocket %s %d\n", host.c_str(), port);
		return 0;
	}

	s->type = Socket_Type_ToOut;
	s->status = Socket_Status_Reserve;
	s->proxy = proxy;
	s->ip = host;
	s->port = port;

	return s->id;
}

void SocketConnector::setUpTimeOut(uint32_t timeout) {
	m_connTimeout.setTimeout(timeout);
	m_connTimeout.setFunction(std::bind(&SocketConnector::doConnectTimeOut, this, std::placeholders::_1));
}

void SocketConnector::doConnectTimeOut(uint32_t id) {
	close(id, CloseType_ConnectFail);
}

void SocketConnector::pushBackPacket(RecvPacket::ptr packet) {
	m_packets.push_back(packet);
}

}

