#ifndef _SERVER_H_
#define _SERVER_H_

#include "application.h"

using namespace Storm;

class Server : public Application {
private:
	bool initialize();
	void loop();
	void destroy();

private:
	LockQueue<int> m_msgQueue;
	int m_count;
};

#endif
