#include "application.h"

#include <signal.h>
#include <stdlib.h>

#include "util/util_option.h"
#include "util/util_config.h"
#include "util/util_file.h"

#include "storm/log.h"

#include "common_header.h"

namespace Storm {

const static uint32_t kDefaultThreadNum = 1;
const static uint32_t kDefaultConnections = 65535;
const static uint32_t kDefaultQueueLen = 100000;
const static uint32_t kEmptyTimeOut = 15;
const static uint32_t kDefaultKeepAliveTime = 60;
const static uint32_t kDefaultTimeOut = 60;

bool g_exit = false;

static string g_pidFile;

static void sighandler(int /*sig*/)
{
	g_exit = true;
}

void Application::terminate() {
	m_connector->terminate();
	m_sockServer->terminate();
}

int Application::run(int argc, char** argv) {
	try {
		m_sockServer = new SocketServer();
		m_connector = new SocketConnector();

		parseConfig(argc, argv);
		savePidFile();
		signal(SIGINT, sighandler);
		signal(SIGTERM, sighandler);
		signal(SIGPIPE, SIG_IGN);

		displayServer();

		if (!initialize()) {
			return -1;
		}

		m_sockServer->show();
		m_sockServer->start();
		LOG("size of Server %ld\n", sizeof(*m_sockServer));

		m_connector->start();
		while (!m_sockServer->isTerminate() && !m_connector->isTerminate()) {
			if (g_exit) {
				terminate();
			} else {
				loop();
			}
		}

		destroy();
		removePidFile();
		LOG("normal exit\n");

	} catch (std::exception& e) {
		cerr << "server error: " << e.what() << endl;
		return -1;
	}

	return 0;
}

void Application::parseConfig(int argc, char** argv) {
	COption option;
	option.parse(argc, argv);

	string configFile = option.getConfigFile();
	if (configFile.empty()) {
		configFile = "app.conf";
	}

	CConfig config;
	config.parseFile(configFile);

	const CConfig& svrCfg = config.getSubConfig("server");
	parseServerConfig(svrCfg);

	killOldProcess();
	if (option.isStop()) {
		exit(0);
	}
}

void Application::parseServerConfig(const CConfig& cfg) {
	m_config.appName = cfg.getCfg("app");
	m_config.serverName = cfg.getCfg("server");
	g_pidFile = m_config.serverName + ".pid";

	map<string, ServiceConfig>& allService = m_config.services;
	const map<string, CConfig>& servicesCfg = cfg.getAllSubConfig();
	for (map<string, CConfig>::const_iterator it = servicesCfg.begin(); it != servicesCfg.end(); ++it) {
		const CConfig& serviceCfg = it->second;
		string serviceName = serviceCfg.getCfg("service");
		if (allService.find(serviceName) != allService.end()) {
			throw std::runtime_error("duplicate service: " + serviceName);
		}
		ServiceConfig& stCfg = allService[serviceName];
		stCfg.serviceName = serviceName;
		stCfg.host = serviceCfg.getCfg("host");
		stCfg.port = serviceCfg.getCfg<uint32_t>("port");
		stCfg.threadNum = serviceCfg.getCfg("thread", kDefaultThreadNum);
		stCfg.maxConnections = serviceCfg.getCfg("maxConnections", kDefaultConnections);
		stCfg.maxQueueLen = serviceCfg.getCfg("maxQueueLen", kDefaultQueueLen);
		stCfg.keepAliveTime = serviceCfg.getCfg("keepAliveTime", kDefaultKeepAliveTime);
		stCfg.emptyConnTimeOut = serviceCfg.getCfg("emptyConnTimeOut", kEmptyTimeOut);
		stCfg.queueTimeout = serviceCfg.getCfg("queueTimeout", kDefaultTimeOut);
	}
}

void Application::displayServer() {
	cout << "AppName " << m_config.appName << endl;
	cout << "ServerName " << m_config.serverName << endl;

	map<string, ServiceConfig>& allService = m_config.services;
	for (map<string, ServiceConfig>::const_iterator it = allService.begin(); it != allService.end(); ++it) {
		const ServiceConfig& cfg = it->second;
		cout << endl;
		cout << "Service: " << cfg.serviceName << endl;
		cout << "\t Host: " << cfg.host << endl;
		cout << "\t Port: " << cfg.port << endl;
		cout << "\t ThreadNum: " << cfg.threadNum << endl;
		cout << "\t MaxConnections: " << cfg.maxConnections << endl;
		cout << "\t MaxQueueLen: " << cfg.maxQueueLen << endl;
		cout << "\t QueueTimeOut: " << cfg.queueTimeout << endl;
	}
}

void Application::savePidFile() {
	int pid = getpid();
	LOG("pid %d\n", pid);
	UtilFile::saveToFile(g_pidFile, UtilString::tostr(pid) + "\n");
}

void Application::removePidFile() {
	UtilFile::removeFile(g_pidFile);
}

void Application::killOldProcess() {
	string pidStr = UtilFile::loadFromFile(g_pidFile);
	if (pidStr.empty()) {
		cout << "no server running" << endl;
		return;
	}
	int pid = UtilString::strto<int>(pidStr);
	cout << "stoping server, pid " << pid << endl;

	int ret = kill(pid, 15);
	if (ret == ESRCH) {
		cout << "permission not enough" << endl;
		return;
	} else if (ret == EPERM) {
		cout << "server not running" << endl;
		removePidFile();
		return;
	} else if (ret == 0) {
		while (UtilFile::isFileExists(g_pidFile)) {
			usleep(100 * 1000);
		}
	}

	cout << "server exit " << endl;
}

}
