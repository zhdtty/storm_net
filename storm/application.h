#ifndef _STORM_APPLICATION_H_
#define _STORM_APPLICATION_H_

#include <unistd.h>

#include "net/socket_server.h"
#include "net/socket_connector.h"
#include "util/util_config.h"
#include "util/util_log.h"
#include "app_config.h"

namespace Storm {
class Application {
public:
	int run(int argc, char** argv);

	static void terminate();

	//默认initialize之前日志同步，initialize之后日志异步
	//开发阶段可以让日志同步输出，用于调试crash类bug
	static void setLogSync(bool sync);

protected:
	virtual bool initialize() {return true;};
	virtual void destroy() {};
	virtual void loop() {sleep(1000);};

	template<typename T>
	void addService(const string& serviceName) {
		map<string, ServiceConfig>& allService = m_config.services;
		map<string, ServiceConfig>::iterator it = allService.find(serviceName);
		if (it == allService.end()) {
			throw std::runtime_error("cannot find service config, service: " + serviceName);
		}
		bool suc = m_sockServer->addListener<T>(it->second);
		if (suc == false) {
			exit(0);
		}
	}

private:
	void parseConfig(int argc, char** argv);
	void parseServerConfig(const CConfig& cfg);
	void parseClientConfig(const CConfig& cfg);

	void savePidFile();
	void removePidFile();
	void killOldProcess();

	void displayServer();

protected:
	ServerConfig m_config;
	ClientConfig m_clientConfig;
	static SocketServer* m_sockServer;
	static SocketConnector* m_connector;
};

void setDefaultConfigFile(const string& file);
}

#endif
