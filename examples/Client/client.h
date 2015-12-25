#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "application.h"

using namespace Storm;

class Client : public Application {
private:
	bool initialize();
	void loop();
	void destroy();

private:
	LockQueue<int> m_msgQueue;
	int m_count;
};

#endif
