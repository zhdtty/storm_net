#include "service_proxy.h"


#include <iostream>

#include "socket_connector.h"

#include "protocol.h"
#include "proto/rpc_proto.pb.h"

#include "util/util_string.h"
#include "util/util_time.h"

namespace Storm {

ServiceProxy::ServiceProxy(SocketConnector* connector):m_connector(connector) {
	m_sequeue = 0;
	m_protocol = std::bind(FrameProtocolLen::decode, std::placeholders::_1, std::placeholders::_2);
	m_timeout.setTimeout(5);
	m_timeout.setFunction(std::bind(&ServiceProxy::doTimeClose, this, std::placeholders::_1));
}

void ServiceProxy::doTimeOut() {
	uint32_t now = UtilTime::getNow();
	ScopeMutex<Mutex> lock(m_messMutex);
	m_timeout.timeout(now);
}

void ServiceProxy::doTimeClose(uint32_t id) {
	m_connector->close(id, CloseType_Timeout);
}

bool ServiceProxy::parseFromString(const string& config) {
	vector<uint32_t> endPoints;

	string::size_type pos = config.find('@');
	if (pos != string::npos) {
		m_needLocator = false;	
		m_objName = config.substr(0, pos);
		vector<string> endpoints = UtilString::splitString(config.substr(pos + 1), ":");
		for (uint32_t i = 0; i < endpoints.size(); ++i) {
			string desc = endpoints[i];
			vector<string> v = UtilString::splitString(desc, " \t\r\n");
			if (v.size() < 3 || v.size() % 2 != 1) {
				return false;
			}
			//只支持tcp
			if (v[0] != "tcp") {
				return false;
			}

			string host;
			uint32_t port = 0;
			for (unsigned j = 1; j < v.size(); j += 2) {
				const string &opt = v[j];
				const string &arg = v[j + 1];
				if (opt.length() != 2 || opt[0] != '-') {
					return false;
				}
				if (opt[1] == 'h') {
					host = arg;
				} else if (opt[1] == 'p') {
					port = UtilString::strto<uint32_t>(arg);
				} else if (opt[1] == 't') {
				} else {
					continue;
				}
			}
			if (host.empty() || port == 0) {
				return false;
			}

			string key = m_objName + "@" + host + ":" + UtilString::tostr(port);
			map<string, uint32_t>::iterator it = m_clients.find(key);
			uint32_t id =0;
			if (it == m_clients.end()) {
				id = m_connector->getNewClientSocket(this, host, port);
				m_clients.insert(make_pair(key, id));
			} else {
				id = it->second;
			}
			endPoints.push_back(id);
		}
	} else {
		m_objName = config;
		m_needLocator = true;
		//TODO 同步调用locator
	}

	setEndPoints(endPoints);
	return true;
}

//TODO
void ServiceProxy::updateEndPoints() {
	if (!m_needLocator) {
		return;
	}
}

void ServiceProxy::setEndPoints(vector<uint32_t>& eps) {
	ScopeMutex<Mutex> lock(m_epMutex);
	if (!eps.empty()) {
		m_endPoints.swap(eps);
	}
}

void ServiceProxy::delEndPoint(uint32_t id) {
	ScopeMutex<Mutex> lock(m_epMutex);
	//如果是指定ip访问的，不要删掉，即使出了故障也要尝试
	if (!m_needLocator) {
		return;
	}
	for (vector<uint32_t>::iterator it = m_endPoints.begin(); it != m_endPoints.end(); ++it) {
		if (*it == id) {
			m_endPoints.erase(it);
			break;
		}
	}
}

void ServiceProxy::onData(uint32_t id, IOBuffer::ptr buffer) {
	while (1) {
		string  out;
		int code = m_protocol(buffer, out);
		if (code == Packet_Less) {
			break;
		} else if (code == Packet_Error) {
			m_connector->close(id);
			break;
		}

		RecvPacket::ptr pack(new RecvPacket);
		pack->id = id;
		pack->proxy = this;
		pack->type = Net_Packet;
		swap(pack->buffer, out);

		m_connector->pushBackPacket(pack);
	}
}

void ServiceProxy::onClose(uint32_t id, uint32_t closeType) {
	RecvPacket::ptr pack(new RecvPacket);
	pack->id = id;
	pack->proxy = this;
	pack->type = Net_Close;
	pack->closeType = closeType;

	m_connector->pushBackPacket(pack);
}

void ServiceProxy::doRequest(RecvPacket::ptr pack) {
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
	finishInvoke(mess);
}

void ServiceProxy::doClose(RecvPacket::ptr pack) {
	LOG("onClose! id %d, closeType %d\n", pack->id, pack->closeType);

	if (pack->closeType == CloseType_ConnectFail) {
		delEndPoint(pack->id);
	}

	uint32_t id = pack->id;
	ScopeMutex<Mutex> lock(m_messMutex);
	for (map<uint32_t, ReqMessage*>::iterator it = m_reqMessages.begin(); it != m_reqMessages.end(); ) {
		if (id != it->second->sockId) {
			it++;
			continue;
		}
		m_timeout.del(it->first);
		ReqMessage* mess = it->second;
		m_reqMessages.erase(it++);

		mess->status = RespStatus_Error;
		if (pack->closeType == CloseType_Timeout) {
			mess->status = RespStatus_TimeOut;
		} else if (pack->closeType == CloseType_ConnectFail) {
			mess->status = RespStatus_UnReachableHost;
		}
		finishInvoke(mess);
	}
}

void ServiceProxy::terminate() {
	ScopeMutex<Mutex> lock(m_messMutex);
	for (map<uint32_t, ReqMessage*>::iterator it = m_reqMessages.begin(); it != m_reqMessages.end(); ) {
		m_timeout.del(it->first);
		ReqMessage* mess = it->second;
		m_reqMessages.erase(it++);

		mess->status = RespStatus_TimeOut;
		finishInvoke(mess);
	}
}

void ServiceProxy::setReqMessage(uint32_t requestId, ReqMessage* mess) {
	ScopeMutex<Mutex> lock(m_messMutex);
	m_reqMessages[requestId] = mess;
	uint32_t now = UtilTime::getNow();
	m_timeout.add(requestId, now);
}

ReqMessage* ServiceProxy::getAndDelReqMessage(uint32_t requestId) {
	ScopeMutex<Mutex> lock(m_messMutex);
	map<uint32_t, ReqMessage*>::iterator it = m_reqMessages.find(requestId);
	if (it == m_reqMessages.end()) {
		return NULL;
	}
	ReqMessage* temp = it->second;
	m_reqMessages.erase(it);
	m_timeout.del(requestId);
	return temp;
}

void ServiceProxy::doInvoke(ReqMessage* req) {
	uint32_t invokeType = req->invokeType;
	uint32_t requestId = m_sequeue.fetch_add(1);
	req->req.set_request_id(requestId);

	IOBuffer::ptr packet = FrameProtocolLen::encode(req->req);
	if (invokeType != InvokeType_OneWay) {
		setReqMessage(requestId, req);
	}

	uint32_t sendId = selectEndPoint(requestId);
	if (sendId == 0) {
		LOG("no endpoint %s\n", m_objName.c_str());
		req->status = RespStatus_UnReachableHost;
		finishInvoke(req);
		return;
	}

	req->sockId = sendId;
	m_connector->send(sendId, packet);

	//异步请求req后面不一定用了，所以这里把invokeType复制
	if (invokeType == InvokeType_Sync) {
		ScopeMutex<Notifier> lock(req->handler->m_notifier);
		if (req->back == false) {
			req->handler->m_notifier.wait();
		}
	}
}

// 1.同步请求只需唤醒调用线程 2.异步请求调用回调函数
void ServiceProxy::finishInvoke(ReqMessage* req) {
	if (req->invokeType == InvokeType_Sync) {
		ScopeMutex<Notifier> lock(req->handler->m_notifier);
		req->back = true;
		req->handler->m_notifier.signal();
		LOG("finishInvoke\n");
		return;
	} else if (req->invokeType == InvokeType_Async) {
		//TODO 提供一个接受队列，塞到队列里，实现callback指定线程调用
		req->cb->dispatch(req);
	}
}

uint32_t ServiceProxy::selectEndPoint(uint32_t requestId) {
	ScopeMutex<Mutex> lock(m_epMutex);
	if (m_endPoints.empty()) {
		return 0;
	}
	return m_endPoints[requestId %  m_endPoints.size()];
}

ReqMessage* newRequest(InvokeType type, ServiceProxyCallBackPtr cb) {
	ReqMessage* req = new ReqMessage;

	req->invokeType = type;
	req->cb = cb;

	if (type == InvokeType_Sync) {
		req->handler = new RecvPacketHandler;
	} else if (!cb) {
		req->invokeType = InvokeType_OneWay;
	}
	req->req.set_invoke_type(req->invokeType);

	return req;
}

void delRequest(ReqMessage* mess) {
	if (mess->handler) {
		delete mess->handler;
		mess->handler = NULL;
	}
	if (mess->resp) {
		delete mess->resp;
		mess->resp = NULL;
	}
	delete mess;
}

}
