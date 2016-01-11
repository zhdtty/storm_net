#ifndef _STORM_UTIL_LOG_H_
#define _STORM_UTIL_LOG_H_

#include <memory>
#include <sstream>

#include "util_string.h"
#include "util_time.h"
#include "noncopyable.h"

namespace Storm {

enum LogType {
	LogType_Roll,
	LogType_Day,
	LogType_Hour,
};

enum LogLevel {
	LogLevel_Debug,
	LogLevel_Info,
	LogLevel_Error,
	LogLevel_None,
};

struct LogData {
	typedef std::shared_ptr<LogData> ptr;
	uint32_t time;
	string content;
};

class LogManager {
public:
	static void initLog(const string& path, const string fileNamePrefix);
	static void doLog(const string& logName, LogType logType, LogData::ptr logData);

};

class LogStream : public noncopyable {
public:
	LogStream(const string& logName, LogType logType);
	LogStream(const string& logName, LogType logType, LogLevel logLevel, bool isFrameWork, const string& fileName, int line, const string& func);
	~LogStream();

	ostringstream& stream() {
		return m_oss;
	}
private:
	string m_logName;
	LogType m_logType;
	ostringstream m_oss;
};

const char *_briefLogFileName(const char *name);

#define LOG_DEBUG if (1) \
	LogStream("", Storm::LogType_Roll, Storm::LogLevel_Debug, false, __FILE__, __LINE__, __FUNCTION__).stream()

#define LOG_INFO if (1) \
	LogStream("", Storm::LogType_Roll, Storm::LogLevel_Info, false, __FILE__, __LINE__, __FUNCTION__).stream()

#define LOG_ERROR if (1) \
	LogStream("", Storm::LogType_Roll, Storm::LogLevel_Error, false, __FILE__, __LINE__, __FUNCTION__).stream()

#define STORM_DEBUG if (1) \
	LogStream("", Storm::LogType_Roll, Storm::LogLevel_Debug, true, __FILE__, __LINE__, __FUNCTION__).stream()

#define STORM_INFO if (1) \
	LogStream("", Storm::LogType_Roll, Storm::LogLevel_Info, true, __FILE__, __LINE__, __FUNCTION__).stream()

#define STORM_ERROR if (1) \
	LogStream("", Storm::LogType_Roll, Storm::LogLevel_Error, true, __FILE__, __LINE__, __FUNCTION__).stream()

}

#endif
