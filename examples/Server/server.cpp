#include "server.h"

#include <stdio.h>

#include "log.h"

#include "GameServiceImp.h"

Server g_server;

bool Server::initialize() {
	addService<GameServiceImp>("GameService");

	return true;
}

void Server::destroy() {

}

void Server::loop() {
	int num;
	m_msgQueue.pop_front(num, 1 * 1000);
}

int main(int argc, char** argv) {	
	setDefaultConfigFile("server.conf");
	return g_server.run(argc, argv);
}

