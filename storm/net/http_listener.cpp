#include "http_listener.h"

#include "util/util_log.h"
#include "protocol.h"

namespace Storm {

void HttpListener::setHandle(SocketHandler* handler) {
	m_handler = handler;
	m_handler->setProtocol(std::bind(FrameProtocolHttp::decode, std::placeholders::_1, std::placeholders::_2));
}

void HttpListener::doRequest(Connection::ptr conn) {
	LOG_DEBUG << conn->buffer.size();
	LOG_DEBUG << endl << conn->buffer;

	string msg = "Hello World!";
	ostringstream oss;
	oss << "HTTP/1.1 200 OK\r\nContent-Length:" << msg.size()
		<< "\r\n\r\n"
		<< msg;

	send(conn->id, oss.str());
}

}
