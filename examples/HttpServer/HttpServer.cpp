#include "HttpServer.h"
#include "net/http_listener.h"

HttpServer g_server;

bool HttpServer::initialize() {
	addService<HttpListener>("HttpService");

	return true;
}

void HttpServer::destroy() {

}

int main(int argc, char** argv) {	
	return g_server.run(argc, argv);
}

