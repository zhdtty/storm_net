#include "socket_listener.h"

#include <time.h>

#include "socket_server.h"
#include "protocol.h"
#include "util/util_time.h"
#include "util/util_log.h"
#include "net/request.h"

#include <iostream>


namespace Storm {

const char* etos(Socket_CloseType closeType) {
	switch (closeType) {
		case CloseType_Server:
			return "CloseType_Server";
		case CloseType_Client:
			return "CloseType_Client";
		case CloseType_Timeout:
			return "CloseType_Timeout";
		case CloseType_EmptyTimeout:
			return "CloseType_EmptyTimeout";
		case CloseType_Packet_Error:
			return "CloseType_Packet_Error";
		case CloseType_ConnectFail:
			return "CloseType_ConnectFail";
	}
	return "unknown";
}

SocketListener::SocketListener():m_handler(NULL), m_sockServer(NULL) {
}

void SocketListener::setHandle(SocketHandler* handler) {
	m_handler = handler;
	m_handler->setProtocol(std::bind(FrameProtocolLen::decode, std::placeholders::_1, std::placeholders::_2));
}

void SocketListener::setProtocol(ProtocolType protocol) {
	m_handler->setProtocol(protocol);
}

void SocketListener::doRequest(Connection::ptr pack) {
	int ret = 0;
	RpcRequest req;
	RpcResponse resp;

	//解析RpcRequest
	if (!req.ParseFromString(pack->buffer)) {
		STORM_ERROR << "rpc request error";
		m_sockServer->close(pack->id, CloseType_Packet_Error);
		return;
	}

	//业务逻辑
	ret = doRpcRequest(pack, req, resp);

	if (req.invoke_type() == InvokeType_OneWay) {
		return;
	}

	//回包
	resp.set_ret(ret);
	resp.set_proto_id(req.proto_id());
	resp.set_request_id(req.request_id());

	IOBuffer::ptr respBuf = FrameProtocolLen::encode(resp);
	m_sockServer->send(pack->id, respBuf);
}

void SocketListener::send(int id, IOBuffer::ptr buffer) {
	m_sockServer->send(id, buffer);
}

void SocketListener::send(int id, const string& str) {
	IOBuffer::ptr buffer(new IOBuffer(str));
	m_sockServer->send(id, buffer);
}

void SocketListener::start() {
	m_thread = std::thread(&SocketListener::process, this);
}

void SocketListener::showLen() {
	STORM_DEBUG << "queue " << m_handler->m_packets.size();
}

void SocketListener::terminate() {
	destroy();
	m_thread.join();
}

void SocketListener::process() {
	Connection::ptr pack;
	while (!m_handler->m_term) {
		if (m_handler->m_packets.pop_front(pack, -1)) {
			if (pack->type == Net_Close) {
				doClose(pack);
			} else if (pack->type == Net_Packet) {
				doRequest(pack);
			}
		}
	}
}

/*
 *	SocketHandler
 */

SocketHandler::SocketHandler():m_term(false), m_sockServer(NULL) {
	m_protocol = std::bind(FrameProtocolLen::decode, std::placeholders::_1, std::placeholders::_2);
}

void SocketHandler::setUpTimeOut() {
	m_conList.setTimeout(m_config.emptyConnTimeOut);
	m_conList.setFunction(std::bind(&SocketHandler::doEmptyClose, this, std::placeholders::_1));

	m_timelist.setTimeout(m_config.keepAliveTime);
	m_timelist.setFunction(std::bind(&SocketHandler::doTimeClose, this, std::placeholders::_1));
}

void SocketHandler::checkTimeOut() {
	uint32_t now = UtilTime::getNow();
	m_conList.timeout(now);
	m_timelist.timeout(now);
}

//TODO
void SocketHandler::headBeat() {

}

void SocketHandler::doTimeClose(uint32_t id) {
	m_sockServer->close(id, CloseType_Timeout);
}

void SocketHandler::doEmptyClose(uint32_t id) {
	m_sockServer->close(id, CloseType_EmptyTimeout);
}

void SocketHandler::onAccept(int id, int fd, const string& ip, int port) {
	STORM_INFO << m_config.serviceName << " accept " << id << " " << fd << " " << ip << ":" << port;
	uint32_t now = UtilTime::getNow();
	if (m_conList.size() >= m_config.maxConnections) {
		STORM_ERROR << "max Connection";
		m_sockServer->close(id, CloseType_Server);
	} else {
		m_conList.add(id, now);
	}
}

void SocketHandler::onClose(int id, int fd, const string& ip, int port, int closeType) {
	STORM_INFO << m_config.serviceName << " close " << id << " " << fd << " " << ip << ":" << port
			   << ", " << etos((Socket_CloseType)closeType);
	m_conList.del(id);
	m_timelist.del(id);
	Connection::ptr pack(new Connection);
	pack->type = Net_Close;
	pack->id = id;
	pack->fd = fd;
	pack->port = port;
	pack->ip = ip;
	pack->closeType = closeType;

	m_packets.push_back(pack);
}

void SocketHandler::onData(int id, int fd, const string& ip, int port, IOBuffer::ptr buffer) {
	uint32_t now = UtilTime::getNow();
	m_conList.del(id);
	m_timelist.add(id, now);
	while (1) {
		string  out;
		int code = m_protocol(buffer, out);
		if (code == Packet_Less) {
			break;
		} else if (code == Packet_Error) {
			STORM_ERROR << "packet error " << id << " " << fd << " " << buffer->getSize();
			m_sockServer->close(id);
			break;
		}
		Connection::ptr pack(new Connection);
		pack->type = Net_Packet;
		pack->id = id;
		pack->fd = fd;
		pack->port = port;
		pack->ip = ip;
		swap(pack->buffer, out);

		if (m_packets.size() > m_config.maxQueueLen) {
			STORM_ERROR << "queue size over max";
			return;
		}
		m_packets.push_back(pack);
	}
}

void SocketHandler::start() {
	for (uint32_t i = 0; i < m_listeners.size(); ++i) {
		m_listeners[i]->start();
	}
}

void SocketHandler::terminate() {
	m_term = true;
	m_packets.wakeup();
	for (uint32_t i = 0; i < m_listeners.size(); ++i) {
		m_listeners[i]->terminate();
	}
}

}
