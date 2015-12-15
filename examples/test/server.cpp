#include "server.h"

#include <stdio.h>

#include "storm/log.h"

#include "GameService.h"

Server g_server;

bool Server::initialize() {
	m_count = 0;
	addService<GameService>("GameService");
	//addService<SocketListener>("EchoService1");

	return true;
}

void Server::destroy() {

}

void Server::loop() {
	int num;
	m_msgQueue.pop_front(num, 1 * 1000);
}

int main(int argc, char** argv) {
	 return g_server.run(argc, argv);
}

