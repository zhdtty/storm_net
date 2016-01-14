#include "application.h"

#include <signal.h>
#include <stdlib.h>

#include "util/util_option.h"
#include "util/util_config.h"
#include "util/util_file.h"
#include "util/util_log.h"

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
static string g_configFile;
static COption g_option;
static bool g_syncLog = false;

SocketServer* Application::m_sockServer = NULL;
SocketConnector* Application::m_connector = NULL;

static void sighandler(int /*sig*/) {
	g_exit = true;
}

void greenOutput(const string& content) {
	cout << "\033[1;32m" << content <<  "\033[0m" << endl;
}

void redOutput(const string& content) {
	cout << "\033[1;31m" << content <<  "\033[0m" << endl;
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

		killOldProcess();

		LogManager::initLog("", m_config.appName + "." + m_config.serverName);

		if (g_option.isStop()) {
			exit(0);
		}
		savePidFile();
		cout << "starting server, pid: " << getpid() << endl;
		LOG_DEBUG << "starting server, pid: " << getpid();

		signal(SIGINT, sighandler);
		signal(SIGTERM, sighandler);
		signal(SIGPIPE, SIG_IGN);

		displayServer();

		m_connector->start(m_clientConfig);

		if (!initialize()) {
			redOutput("start server failed");
			return -1;
		}
		LogManager::setLogSync(g_syncLog);

		//LOG("size of Server %ld\n", sizeof(*m_sockServer));
		//m_sockServer->show();
		m_sockServer->start();
		greenOutput("start server success");
		if (g_option.isDaemon()) {
			int fd = open("/dev/null", O_RDWR );
			if (fd != -1) {
				dup2(fd, 0);
				dup2(fd, 1);
				dup2(fd, 2);
			} else {
				close(0);
				close(1);
				close(2);
			}
		}

		while (!m_sockServer->isTerminate() && !m_connector->isTerminate()) {
			if (g_exit) {
				terminate();
				break;
			}
			loop();
		}

		destroy();

		STORM_INFO << "server normal exit";
		LogManager::finish();
		removePidFile();
	} catch (std::exception& e) {
		STORM_ERROR << "server error " << e.what();
		return -1;
	}

	return 0;
}

void Application::parseConfig(int argc, char** argv) {
	g_option.parse(argc, argv);

	string configFile = g_option.getConfigFile();
	g_configFile = briefLogFileName(argv[0]);
	g_configFile.append(".conf");
	if (configFile.empty()) {
		configFile = g_configFile;
	}

	CConfig config;
	config.parseFile(configFile);

	const CConfig& svrCfg = config.getSubConfig("server");
	parseServerConfig(svrCfg);

	const CConfig& clientCfg = config.getSubConfig("client");
	parseClientConfig(clientCfg);
}

void Application::parseClientConfig(const CConfig& cfg) {
//	m_clientConfig.registeryAddress = cfg.getCfg("registery");
	m_clientConfig.asyncThreadNum = cfg.getCfg<uint32_t>("asyncThread", 1);
	m_clientConfig.connectTimeOut = cfg.getCfg<uint32_t>("connectTimeOut", 3000);
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
	STORM_INFO << "AppName " << m_config.appName;
	STORM_INFO << "ServerName " << m_config.serverName;

	map<string, ServiceConfig>& allService = m_config.services;
	for (map<string, ServiceConfig>::const_iterator it = allService.begin(); it != allService.end(); ++it) {
		const ServiceConfig& cfg = it->second;
		STORM_INFO;
		STORM_INFO << "Service: " << cfg.serviceName;
		STORM_INFO << "\t Host: " << cfg.host;
		STORM_INFO << "\t Port: " << cfg.port;
		STORM_INFO << "\t ThreadNum: " << cfg.threadNum;
		STORM_INFO << "\t MaxConnections: " << cfg.maxConnections;
		STORM_INFO << "\t MaxQueueLen: " << cfg.maxQueueLen;
		STORM_INFO << "\t QueueTimeOut: " << cfg.queueTimeout;
	}
}

void Application::savePidFile() {
	int pid = getpid();
	UtilFile::saveToFile(g_pidFile, UtilString::tostr(pid) + "\n");
}

void Application::removePidFile() {
	UtilFile::removeFile(g_pidFile);
}

void Application::killOldProcess() {
	string pidStr = UtilFile::loadFromFile(g_pidFile);
	if (pidStr.empty()) {
		return;
	}
	int pid = UtilString::strto<int>(pidStr);
	cout << "stoping old server, pid " << pid << endl;

	int ret = kill(pid, 15);
	if (ret == 0 || errno == 0) {
		cout << "old server exiting ..." << endl;
		while (UtilFile::isFileExists(g_pidFile)) {
			usleep(500 * 1000);
		}
	} else if (errno == ESRCH) {
		redOutput("permission not enough");
		return;
	} else if (errno == EPERM) {
		redOutput("server not running");
		removePidFile();
		return;
	}

	cout << "old server exited" << endl;
}

void Application::setLogSync(bool sync) {
	g_syncLog = sync;
}

void setDefaultConfigFile(const string& file) {
	g_configFile = file;
}

}
