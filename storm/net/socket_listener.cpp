#include "socket_listener.h"

#include <time.h>

#include "socket_server.h"
#include "protocol.h"
#include "util/util_time.h"
#include "net/request.h"

#include <iostream>


namespace Storm {

SocketListener::SocketListener() {
	setProtocol(boost::bind(FrameProtocolLen::decode, _1, _2));
	m_isRpc = true;
}

void SocketListener::onAccept(int id, int fd, const string& ip, int port) {
	LOG("%s accept %d %d %s:%d\n", m_listenerName.c_str(), id, fd, ip.c_str(), port);
}

void SocketListener::onClose(int id, int fd, const string& ip, int port, int closeType) {
	LOG("%s close %d %d %s:%d, close type %d\n", m_listenerName.c_str(), id, fd, ip.c_str(), port, closeType);
	NetPacket::ptr pack(new NetPacket);
	pack->type = Net_Close;
	pack->id = id;
	pack->fd = fd;
	pack->port = port;
	pack->ip = ip;
	pack->closeType = closeType;

	m_msg.push_back(pack);
	m_handle->wakeup();
}

void SocketListener::onData(int id, int fd, const string& ip, int port, IOBuffer::ptr buffer) {
	while (1) {
		string  out;
		int code = m_protocol(buffer, out);
		if (code == Packet_Less) {
			break;
		} else if (code == Packet_Error) {
			m_sockServer->close(id);
			break;
		}
		NetPacket::ptr pack(new NetPacket);
		pack->type = Net_Packet;
		pack->id = id;
		pack->fd = fd;
		pack->port = port;
		pack->ip = ip;
		swap(pack->buffer, out);
//		LOG("packet %s\n", out->getHead());

		m_msg.push_back(pack);
		m_handle->wakeup();
	}
}

void SocketListener::doRequest(NetPacket::ptr pack) {
	int ret = 0;
	RpcRequest req;
	RpcResponse resp;

	do {
		if (!req.ParseFromString(pack->buffer)) {
			LOG("ParseFromString error\n");
			//TODO 错误码
			ret = -1;
			break;
		}
		ret = doRpcRequest(pack, req, resp);
		resp.set_proto_id(req.proto_id());
		resp.set_request_id(req.request_id());

	} while (0);

	resp.set_ret(ret);

	if (req.invoke_type() != InvokeType_OneWay) {
		IOBuffer::ptr respBuf = FrameProtocolLen::encode(resp);
		m_sockServer->send(pack->id, respBuf);
	}
}

void SocketListener::send(int id, IOBuffer::ptr buffer) {
	m_sockServer->send(id, buffer);
}

void SocketListener::send(int id, const string& str) {
	IOBuffer::ptr buffer(new IOBuffer(str));
	m_sockServer->send(id, buffer);
}

bool SocketListener::checkIpValid(const string& sHost) {
	return true;
}

void SocketListener::process() {
	NetPacket::ptr pack;
	while (m_msg.pop_front(pack, 0)) {
		if (pack->type == Net_Close) {
			//LOG("%s close %d %d %s:%d, close type %d\n", m_listenerName.c_str(), id, fd, ip.c_str(), port, closeType);
			doClose(pack);
		} else if (pack->type == Net_Packet) {
			doRequest(pack);
		}
	}
}

/*
 *	SocketHandle
 */

void SocketHandle::start() {
	for (uint32_t i = 0; i < m_threadNum; ++i) {
		Thread::ptr t(new Thread());
		m_threads.push_back(t);
		t->start(boost::bind(&SocketHandle::loop, this));
	}
}

void SocketHandle::loop() {
	while (!m_term) {
		if (isAllEmpty()) {
			ScopeMutex<Notifier> lock(m_notifier);
			m_notifier.wait();
		}
		for (vector<SocketListener::ptr>::iterator it = m_listeners.begin(); it != m_listeners.end(); ++it) {
			(*it)->process();
		}
	}
}

void SocketHandle::terminate() {
	m_term = true;
	wakeupAll();
	for (uint32_t i = 0; i < m_threads.size(); ++i) {
		m_threads[i]->join();
	}
}

void SocketHandle::wakeup() {
	ScopeMutex<Notifier> lock(m_notifier);
	m_notifier.signal();
}

void SocketHandle::wakeupAll()  {
	ScopeMutex<Notifier> lock(m_notifier);
	m_notifier.broadcast();
}

bool SocketHandle::isAllEmpty() {
	for (vector<SocketListener::ptr>::iterator it = m_listeners.begin(); it != m_listeners.end(); ++it) {
		if ((*it)->m_msg.empty() == false) {
			return false;
		}
	}
	return true;
}

bool SocketHandle::addListener(const string& listenerName, SocketListener::ptr listener) {
	for (vector<SocketListener::ptr>::iterator it = m_listeners.begin(); it != m_listeners.end(); ++it) {
		if ((*it)->m_listenerName == listenerName) {
			return false;
		}
	}
	m_listeners.push_back(listener);
	return true;
}

void SocketHandle::setThreadNum(uint32_t threadNum) {
	if (threadNum > kMaxThreadNum) {
		threadNum = kMaxThreadNum;
	}
	m_threadNum = threadNum;
}

}
