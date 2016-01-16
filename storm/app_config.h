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
	uint32_t keepAliveTime;
	uint32_t emptyConnTimeOut;
	uint32_t queueTimeout;
};

struct ServerConfig {
	std::string appName;
	std::string serverName;

	std::string basePath;
	std::string dataPath;
	std::string localIp;
	std::string logPath;

	std::string local;
	std::string node;
	std::string log;
	std::string config;
	std::string notify;
	std::string configFile;
	int         reportFlow;
	int         isCheckSet;

	uint32_t logNum;
	uint64_t logSize;
	uint32_t logLevel;

	map<string, ServiceConfig> services;
};

struct ClientConfig {
	string registeryAddress;
	uint32_t connectTimeOut;
	uint32_t asyncThreadNum;
};

}

#endif
