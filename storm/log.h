#ifndef LOG_H_
#define LOG_H_

#include <cstdio>

#define LOG(fmt,...) \
    printf("%s(%d): %s: " fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

#endif
