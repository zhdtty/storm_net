#ifndef LOG_H_
#define LOG_H_

#include <cstdio>
#include "util/util_time.h"

const char *briefLogFileName(const char *name);

#define LOG(fmt,...) \
    printf("%s|%s(%d): %s: " fmt, UtilTime::formatTime(UtilTime::getNow()).c_str(), briefLogFileName(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)

#endif
