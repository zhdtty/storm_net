#include "service_proxy.h"


#include <iostream>

#include "util/util_string.h"
#include "socket_connector.h"

#include "proto/rpc_proto.pb.h"

namespace Storm {

ServiceProxy::ServiceProxy(SocketConnector* connector):m_connector(connector) {

}

bool ServiceProxy::parseFromString(const string& config) {
	m_clients.clear();

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
			SocketClient::ptr client = m_connector->getSocketClient(m_objName, host, port);
			m_clients.push_back(client.get());
		}
	} else {
		m_objName = config;
		m_needLocator = true;
		//TODO 同步调用locator
	}

	return true;
}

ReqMessage* ServiceProxy::newRequest(InvokeType type, ServiceProxyCallBackPtr cb) {
	ReqMessage* req = new ReqMessage;

	req->invokeType = type;
	req->proxy = this;
	req->cb = cb;

	if (type == InvokeType_Sync) {
		req->handler = new RecvPacketHandler;
	} else if (!cb) {
		req->invokeType = InvokeType_OneWay;
	}
	req->req.set_invoke_type(req->invokeType);

	return req;
}

void ServiceProxy::doInvoke(ReqMessage* req) {
	//TODO 选client
	m_clients[0]->sendPacket(req);

	if (req->invokeType == InvokeType_Sync) {
		//TODO 这里永远等下去, 在client里加超时判断
		req->handler->m_notifier.wait();
	}
}

// 1.同步请求只需唤醒调用线程 2.异步请求调用回调函数
void ServiceProxy::finishInvoke(ReqMessage* req) {
	if (req->invokeType == InvokeType_Sync) {
		req->handler->m_notifier.signal();
	} else if (req->invokeType == InvokeType_Async) {
		//TODO 提供一个接受队列，塞到队列里，实现callback指定线程调用
		req->cb->dispatch(req);
	}
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
