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
	static void setLogLevel(LogLevel level, LogLevel stormLevel);
	static void setRollLogInfo(const string& logName, uint32_t maxFileNum, uint64_t maxFileSize);
};

class LogStream : public noncopyable {
public:
	LogStream(const string& logName, LogType logType);
	LogStream(const string& logName, LogType logType, LogLevel logLevel, bool isFrameWork, const string& fileName, int line, const string& func);
	~LogStream();

	ostringstream& stream();
	void prepareTimeStr();
private:
	string m_logName;
	uint32_t m_logTime;
	LogType m_logType;
};

const char *briefLogFileName(const char *name);

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

#define DAY_LOG(file) \
	LogStream(file, Storm::LogType_Day).stream()

#define HOUR_LOG(file) \
	LogStream(file, Storm::LogType_Hour).stream()

}

#endif
