#ifndef _HTTP_SERVER_H_
#define _HTTP_SERVER_H_

#include "application.h"

using namespace Storm;

class HttpServer : public Application {
private:
	bool initialize();
	void destroy();
};

#endif
