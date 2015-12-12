#ifndef _APP_CONFIG_
#define _APP_CONFIG_

namespace Storm {
struct ServiceConfig {
	string serviceName;
	string host;
	uint32_t port;
	uint32_t threadNum;
	uint32_t maxConnections;
	uint32_t maxQueueLen;
	uint32_t queueTimeout;
	string  group;
};

struct ServerConfig {
	std::string appName;
	std::string serverName;
	std::string basePath;
	std::string dataPath;
	std::string localIp;
	std::string logPath;
	int         logSize;
	int         logNum;
	std::string logLevel;
	std::string local;
	std::string node;
	std::string log;
	std::string config;
	std::string notify;
	std::string configFile;
	int         reportFlow;
	int         isCheckSet;

	map<string, ServiceConfig> services;
};
}

#endif
