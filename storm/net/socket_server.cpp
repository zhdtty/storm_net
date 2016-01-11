#include "socket_server.h"

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

#include "util/util_time.h"


#define HASH_ID(id) (((unsigned)id) % MAX_SOCKET)

namespace Storm {
SocketServer::SocketServer()
	:m_exit(false), m_allocId(-1) {
    if (m_poll.invalid()) {
		STORM_ERROR << "create poll failed";
        ::exit(1);
    }
	int fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if (fd < 0) {
		STORM_ERROR << "efd < 0";
		::exit(1);
	}
	m_socket = getNewSocket();
	m_socket->fd = fd;
	m_socket->type = Socket_Type_Notify;
	m_socket->status = Socket_Status_Listen;

	m_poll.addToRead(fd, m_socket);

	m_heatBeatTime = 0;
	m_heatBeatInterval = 60;
}

void SocketServer::show() {
	STORM_DEBUG << "efd " << m_poll.getFd()
				<< ", socket id: " << m_socket->id
				<< ", socket fd: " << m_socket->fd;
}

SocketServer::~SocketServer() {
	exit();
}

void SocketServer::exit() {
	if (m_exit) {
		return ;
	}

    for (int i = 0; i < MAX_SOCKET; ++i) {
		Socket* s = &m_slot[i];
		if (s->status != Socket_Status_Reserve) {
			forceClose(s);
		}
    }

	for (map<string, SocketHandler::ptr>::iterator it = m_handlers.begin(); it != m_handlers.end(); ++it) {
		SocketHandler::ptr handle = it->second;
		handle->terminate();
	}
	m_exit = true;
}

//这个函数测一下，自己在理解一下
int SocketServer::reserveId() {
	for (int i = 0; i < MAX_SOCKET; ++i) {
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

Socket* SocketServer::getNewSocket() {
	int id = reserveId();
	return getSocket(id);
}

Socket* SocketServer::getSocket(int id) {
	if (id < 0) {
		return NULL;
	}
	return &m_slot[HASH_ID(id)];
}

void SocketServer::forceClose(Socket* s, int closeType) {
	if (s->status == Socket_Status_Idle) {
		return;
	}
	assert(s->status != Socket_Status_Reserve);

	//通知
	if (s->type == Socket_Type_FromOut) {
		s->handler->onClose(s->id, s->fd, s->ip, s->port, closeType);
	}

	//把读写缓存放回缓存池
	if (s->readBuffer) {
		s->readBuffer->reset();
		m_bufferPool.put(s->readBuffer);
		//LOG("put to pool %d\n", s->readBuffer->getCapacity());
		s->readBuffer.reset();
	}
	for (list<IOBuffer::ptr>::iterator it = s->writeBuffer.begin(); it != s->writeBuffer.end(); ) {
		s->writeBuffer.erase(it++);
	}

	//关闭
	m_poll.del(s->fd);
	::close(s->fd);
	//LOG("close fd %d %d\n", s->id, s->fd);
	s->status = Socket_Status_Idle;
}

void SocketServer::handleAccept(Socket* s) {
	if (s->type != Socket_Type_Listen) {
		return;
	}
	sockaddr_in sa;
	socklen_t len = sizeof(sa);
	while (1) {
		int connFd = ::accept(s->fd, (sockaddr*)&sa, &len);
		if (connFd < 0) {
			if (errno == EAGAIN) {
				break;
			} else {
				continue;
			}
		}

		Socket* ns = getNewSocket();
		if (ns == NULL) {
			STORM_ERROR << "over max socket num";
			::close(connFd);
			break;
		}
		ns->fd = connFd;
		ns->type = Socket_Type_FromOut;
		ns->status = Socket_Status_Connected;
		ns->handler = s->handler;

    	socketKeepAlive(connFd);
		socketNonBlock(connFd);
		m_poll.addToRead(connFd, ns);

		inet_ntop(AF_INET, (const void*)&sa.sin_addr, m_buffer, sizeof(m_buffer));
		m_buffer[MAX_INFO-1] = '\0';

		ns->ip = m_buffer;
		ns->port = ntohs(sa.sin_port);

		ns->handler->onAccept(ns->id, ns->fd, ns->ip, ns->port);
	}
}

void SocketServer::handleRead(Socket* s) {
	if (s->status != Socket_Status_Connected) {
		return;
	}
	if (!s->readBuffer) {
		s->readBuffer = m_bufferPool.get();
		//LOG("new buffer size %d %d\n", s->readBuffer->getSize(), s->readBuffer->getCapacity());
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
			//LOG("cliet close\n");
			needClose = true;
			break;
		} else if (readSize < 0) {
    		if(errno == EAGAIN) { //没数据可以读
    			break;
    		} else if(errno == EINTR) {
    			continue;
    		} else {
				STORM_INFO << "connection error fd: " << s->fd << " error: " << strerror(errno);
				needClose = true;
    			break;
    		}
		} else {
			buffer->writeN(readSize);
		}
	}
	if (needClose) {
		forceClose(s, CloseType_Client);
	} else if (s->type == Socket_Type_FromOut) {
		s->handler->onData(s->id, s->fd, s->ip, s->port, buffer);
	}
}

void SocketServer::handleWrite(Socket* s) {
	if (s->status != Socket_Status_Connected) {
		return;
	}

	char* d = NULL;
	int size = 0;
	int sendSize = 0;
	bool needClose = false;

	while (1) {
		if (s->writeBuffer.empty()) {
			m_poll.addToWrite(s->fd, s, false);
			break;
		}
		IOBuffer::ptr buffer = s->writeBuffer.front();
		d = buffer->getHead();
		size = buffer->getSize();

		sendSize = ::write(s->fd, d, size);
		if (sendSize == 0) {
			needClose = true;
			//LOG("cliet close\n");
			break;
		} else if (sendSize < 0) {
			if (errno == EAGAIN) {	//不能再写了
				break;
			} else if (errno == EINTR) {
				continue;
			} else {
				STORM_INFO << "send error fd: " << s->fd << ", error: " << strerror(errno);
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
		forceClose(s, CloseType_Client);
	}
}

void SocketServer::poll() {
	while (!m_exit) {
		int num = m_poll.wait(m_event, MAX_EVENT, 100);
		for (int i = 0; i < num; ++i) {
			SocketEvent* event = &m_event[i];
			Socket* s = (Socket*)(event->pUd);
			switch (s->type) {
				case Socket_Type_Notify:
					handleCmd();
					break;
				case Socket_Type_Listen:
					handleAccept(s);
					break;
				case Socket_Type_FromOut:
				{
					if (event->bRead) {
						handleRead(s);
					} 
					if (event->bWrite) {
						handleWrite(s);
					}
					break;
				}
				default:
					STORM_ERROR << "invalid sock type " << s->type;
					break;
			}
		}
		doTimer();
	}
}

void SocketServer::doTimer() {
	for (map<string, SocketHandler::ptr>::iterator it = m_handlers.begin(); it != m_handlers.end(); ++it) {
		SocketHandler::ptr handle = it->second;
		handle->checkTimeOut();
	}

	uint32_t now = UtilTime::getNow();
	if (now >= m_heatBeatTime) {
		for (map<string, SocketHandler::ptr>::iterator it = m_handlers.begin(); it != m_handlers.end(); ++it) {
			it->second->headBeat();
		}
		m_heatBeatTime = now + m_heatBeatInterval;
	}
}

void SocketServer::handleCmd() {
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

void SocketServer::sendSocket(int id, IOBuffer::ptr buffer) {
	Socket* s = getSocket(id);
	if (s == NULL || s->status != Socket_Status_Connected) {
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
				//LOG("cliet close\n");
				break;
			} else if (sendSize < 0) {
				if (errno == EAGAIN) {	//不能再写了
					m_poll.addToWrite(s->fd, s, true);
					break;
				} else if (errno == EINTR) {
					continue;
				} else {
					STORM_INFO << "send error fd: " << s->fd << ", error: " << strerror(errno);
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
		forceClose(s, CloseType_Client);
	} else if (remainSize != 0) {
		s->writeBuffer.push_back(buffer);
	}
}

void SocketServer::closeSocket(int id, uint32_t closeType) {
	Socket* s = getSocket(id);
	if (s == NULL || s->status == Socket_Status_Idle) {
		return;
	}
	forceClose(s, closeType);
}

void SocketServer::start() {
	m_netThread = std::thread(&SocketServer::poll, this);

	for (map<string, SocketHandler::ptr>::iterator it = m_handlers.begin(); it != m_handlers.end(); ++it) {
		SocketHandler::ptr handle = it->second;
		handle->start();
	}
}

void SocketServer::send(int id, IOBuffer::ptr buffer) {
	SocketCmd cmd;
	cmd.type = Socket_Cmd_Send;
	cmd.id = id;
	cmd.buffer = buffer;

	pushCmd(cmd);
}

void SocketServer::close(int id, uint32_t closeType) {
	SocketCmd cmd;
	cmd.id = id;
	cmd.type = Socket_Cmd_Close;
	cmd.closeType = closeType;

	pushCmd(cmd);
}

void SocketServer::terminate() {
	SocketCmd cmd;
	cmd.type = Socket_Cmd_Exit;

	pushCmd(cmd);

	m_netThread.join();
}

inline void SocketServer::pushCmd(const SocketCmd& cmd) {
	m_queue.push_back(cmd);
	uint64_t d = 1;
	::write(m_socket->fd, &d, sizeof(d));
}

}
