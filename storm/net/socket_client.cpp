#include "socket_client.h"

#include <iostream>

#include "protocol.h"
#include "socket_connector.h"

namespace Storm {

SocketClient::SocketClient() {
	setProtocol(boost::bind(FrameProtocolLen::decode, _1, _2));
}

void SocketClient::setReqMessage(ReqMessage* mess) {
	ScopeMutex<Mutex> lock(m_mutex);
	m_reqMessages[mess->req.request_id()] = mess;
}

ReqMessage* SocketClient::getReqMessage(uint32_t requestId) {
	ScopeMutex<Mutex> lock(m_mutex);
	map<uint32_t, ReqMessage*>::iterator it = m_reqMessages.find(requestId);
	if (it == m_reqMessages.end()) {
		return NULL;
	}
	return it->second;
}

ReqMessage* SocketClient::getAndDelReqMessage(uint32_t requestId) {
	ScopeMutex<Mutex> lock(m_mutex);
	map<uint32_t, ReqMessage*>::iterator it = m_reqMessages.find(requestId);
	if (it == m_reqMessages.end()) {
		return NULL;
	}
	ReqMessage* temp = it->second;
	m_reqMessages.erase(it);
	return temp;
}

void SocketClient::delReqMessage(uint32_t requestId) {
	ScopeMutex<Mutex> lock(m_mutex);
	m_reqMessages.erase(requestId);
}

void SocketClient::sendPacket(ReqMessage* req) {
	//1. 生成requestId 2.记录req
	req->req.set_request_id(m_sequeue.fetch_add(1));
	if (req->invokeType != InvokeType_OneWay) {
		setReqMessage(req);
	}

	IOBuffer::ptr packet = FrameProtocolLen::encode(req->req);
	m_connector->send(m_id, packet);
}

void SocketClient::onClose(uint32_t closeType) {
	RecvPacket::ptr pack(new RecvPacket);
	pack->type = RecvType_Close;
	pack->closeType = closeType;
	m_packets.push_back(pack);

	m_connector->wakeup();
}

void SocketClient::onData(IOBuffer::ptr buffer) {
	while (1) {
		string  out;
		int code = m_protocol(buffer, out);
		if (code == Packet_Less) {
			break;
		} else if (code == Packet_Error) {
			m_connector->close(m_id);
			break;
		}

		RecvPacket::ptr pack(new RecvPacket);
		pack->type = RecvType_Packet;
		swap(pack->buffer, out);

		m_packets.push_back(pack);

		m_connector->wakeup();
	}
}

void SocketClient::process() {
	RecvPacket::ptr pack;
	while (m_packets.pop_front(pack, 0)) {
		if (pack->type == RecvType_Close) {
			//LOG("%s close %d %d %s:%d, close type %d\n", m_listenerName.c_str(), id, fd, ip.c_str(), port, closeType);
			doClose(pack);
		} else if (pack->type == RecvType_Packet) {
			doRequest(pack);
		}
	}
}

void SocketClient::doClose(RecvPacket::ptr pack) {
	LOG("onClose!\n");

	//TODO 加锁
	for (map<uint32_t, ReqMessage*>::iterator it = m_reqMessages.begin(); it != m_reqMessages.end(); ) {
		ReqMessage* mess = it->second;
		m_reqMessages.erase(it++);

		mess->status = RespStatus_Error;
		if (pack->closeType == Timeout_Close) {
			mess->status = RespStatus_TimeOut;
		} else if (pack->closeType == Connect_Error) {
			mess->status = RespStatus_UnReachableHost;
		}
		mess->proxy->finishInvoke(mess);
	}
}

void SocketClient::doRequest(RecvPacket::ptr pack) {
	RpcResponse* resp = new RpcResponse();
	if (!resp->ParseFromString(pack->buffer)) {
		LOG("parse response error\n");
		delete resp;
		return;
	}

	uint32_t requestId = resp->request_id();

	ReqMessage* mess = getAndDelReqMessage(requestId);
	if (mess == NULL) {
		LOG("error, unknown requestId\n");
		delete resp;
		return;
	}

	mess->resp = resp;
	mess->status = RespStatus_Ok;
	mess->proxy->finishInvoke(mess);
}

}
