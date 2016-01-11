#include "util_log.h"

#include <string.h>
#include <stdio.h>

namespace Storm {

const char *_briefLogFileName(const char *name) {
	if (name[0] != '/') {
		return name;
	}
	const char* p = strrchr(name, '/');
	if (p != NULL) {
		return p + 1;
	}
	return name;
}

class LogBase {
public:
	LogBase(const string& path, const string fileNamePrefix, const string& fileName)
		:m_path(path), m_fileName(fileName), m_fileSize(0), m_fp(NULL) {

		if (m_path.empty()) {
			m_path = ".";
		}
		if (m_fileName.empty()) {
			m_fullFileName = m_path + "/" + fileNamePrefix;
		} else {
			m_fullFileName = m_path + "/" + fileNamePrefix + "_" + m_fileName;
		}
	}
	virtual ~LogBase() {}

	void writeLog(const LogData& data) {
		if (prepareLogFile(data.time)) {
			uint64_t n = 0;
			n += fwrite(data.content.c_str(), 1, data.content.size(), m_fp);
			m_fileSize += n;
			fflush(m_fp);
		}
	}

	virtual void closeExpiredFile() {}
	virtual bool prepareLogFile(uint32_t time) = 0;

	const string& getLogFullName() const { return m_fullFileName; }

protected:
	string m_path;
	string m_fileName;
	string m_fullFileName;
	uint64_t m_fileSize;
	FILE* m_fp;
};

class RollLog : public LogBase {
public:
	RollLog(const string& path, const string& fileNamePrefix, const string& fileName)
		:LogBase(path, fileNamePrefix, fileName),
		 m_maxFile(10), m_maxSize(10 * 1024 * 1024) {

	}
	~RollLog()  {
		if (m_fp != NULL) {
			fclose(m_fp);
			m_fp = NULL;
		}
	}
protected:
	virtual bool prepareLogFile(uint32_t time) {
		if (m_fp == NULL) {
			string fileName = getLogFullName() + ".log";
			m_fp = fopen(fileName.c_str(), "a+");
			if (m_fp == NULL) {
				return false;
			}
			fseeko(m_fp, 0, SEEK_END);
			m_fileSize = ftello(m_fp);
		}
		return true;
	}
protected:
	uint32_t m_maxFile;
	uint64_t m_maxSize;
};

static string g_logPrefix;

RollLog* g_log = NULL;

void LogManager::initLog(const string& path, const string fileNamePrefix) {
	g_logPrefix = fileNamePrefix;
	g_log = new RollLog("", g_logPrefix, "");
}

void LogManager::doLog(const string& logName, LogType logType, LogData::ptr logData) {
	g_log->writeLog(*logData);
}


const char* g_logLevelName[LogLevel_None] = {
	"DEBUG|",
	"INFO|",
	"ERROR|"
};

LogStream::LogStream(const string& logName, LogType logType)
	:m_logName(logName), m_logType(logType) {

	m_oss << UtilTime::formatTime(UtilTime::getNow()) << "|";
}

LogStream::LogStream(const string& logName, LogType logType, LogLevel logLevel, bool isFrameWork,
					 const string& fileName, int line, const string& func)
	:m_logName(logName), m_logType(logType) {

	m_oss << UtilTime::formatTime(UtilTime::getNow()) << "|" << g_logLevelName[logLevel];
	if (isFrameWork) {
		m_oss << "[STORM]|";
	}
	m_oss << _briefLogFileName(fileName.c_str()) << "(" << line << "):" << func << "|";
}

LogStream::~LogStream() {
	m_oss << endl;

	LogData::ptr logData(new LogData());
	logData->content = m_oss.str();

	LogManager::doLog(m_logName, m_logType, logData);
}

}
