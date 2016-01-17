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
	LogLevel_Debug = 0,
	LogLevel_Info = 1,
	LogLevel_Error = 2,
	LogLevel_Exception = 3,
	LogLevel_None
};

struct LogData {
	typedef std::shared_ptr<LogData> ptr;
	string logName;
	LogType logType;
	uint32_t logTime;
	string content;
};

class LogBase;
class LogManager {
public:
	static void initLog(const string& path, const string fileNamePrefix);
	static void doLog(LogData::ptr logData);
	static void startAsyncThread();

	static void realDoLog(LogData::ptr logData);
	static LogBase* getLog(const string& logName, LogType logType);
	static void finish();

	static void setLogSync(bool sync);
	static void setLogLevel(LogLevel level);
	static void setStormLogLevel(LogLevel level);
	static void setRollLogInfo(const string& logName, uint32_t maxFileNum, uint64_t maxFileSize);

	static LogLevel parseLevel(const string& levelStr);
};

class LogStream : public noncopyable {
public:
	LogStream(const string& logName, LogType logType);
	LogStream(const string& logName, LogType logType, LogLevel logLevel, bool isFrameWork, const string& fileName, int line, const string& func);
	~LogStream();

	ostringstream& stream();

private:
	string m_logName;
	uint32_t m_logTime;
	LogType m_logType;
};

//ERROR日志同时打RollLog和DayLog
class RollDayStream : public noncopyable {
public:
	RollDayStream(const string& logName, LogLevel logLevel, const string& fileName, int line, const string& func);
	~RollDayStream();

	ostringstream& stream();

private:
	string m_logName;
	uint32_t m_logTime;
};

const char *briefLogFileName(const char *name);

extern LogLevel g_level;
extern LogLevel g_stormLevel;

#define LOG_DEBUG if (g_level <= Storm::LogLevel_Debug) \
	Storm::LogStream("", Storm::LogType_Roll, Storm::LogLevel_Debug, false, __FILE__, __LINE__, __FUNCTION__).stream()

#define LOG_INFO if (g_level <= Storm::LogLevel_Info) \
	Storm::LogStream("", Storm::LogType_Roll, Storm::LogLevel_Info, false, __FILE__, __LINE__, __FUNCTION__).stream()

#define LOG_ERROR if (g_level <= Storm::LogLevel_Error) \
	Storm::RollDayStream("error", Storm::LogLevel_Error, __FILE__, __LINE__, __FUNCTION__).stream()

#define LOG_EXCEPTION if (g_level <= Storm::LogLevel_Exception) \
	Storm::RollDayStream("exception", Storm::LogLevel_Exception, __FILE__, __LINE__, __FUNCTION__).stream()

#define STORM_DEBUG if (g_stormLevel <= Storm::LogLevel_Debug) \
	Storm::LogStream("", Storm::LogType_Roll, Storm::LogLevel_Debug, true, __FILE__, __LINE__, __FUNCTION__).stream()

#define STORM_INFO if (g_stormLevel <= Storm::LogLevel_Info) \
	Storm::LogStream("", Storm::LogType_Roll, Storm::LogLevel_Info, true, __FILE__, __LINE__, __FUNCTION__).stream()

#define STORM_ERROR if (g_stormLevel <= Storm::LogLevel_Error) \
	Storm::LogStream("", Storm::LogType_Roll, Storm::LogLevel_Error, true, __FILE__, __LINE__, __FUNCTION__).stream()

#define DAY_LOG(file) \
	Storm::LogStream(file, Storm::LogType_Day).stream()

#define HOUR_LOG(file) \
	Storm::LogStream(file, Storm::LogType_Hour).stream()

}

#endif
