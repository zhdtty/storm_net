#ifndef _STORM_HTTP_LISTENER_H_
#define _STORM_HTTP_LISTENER_H_

#include "socket_listener.h"

namespace Storm {
class HttpListener : public SocketListener {
public:
	virtual bool initialize() {return true;}

	virtual void setHandle(SocketHandler* handler);
	virtual void doClose(Connection::ptr conn) {}
	virtual void doRequest(Connection::ptr conn);
};

}

#endif
