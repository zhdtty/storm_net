#ifndef _STORM_COMMON_HEADER_H_
#define _STORM_COMMON_HEADER_H_

#include <memory>
#include <stdint.h>
#include <string>
#include <vector>
#include <list>
#include <iostream>
#include <thread>

#include <functional>

#include "net/io_buffer.h"
#include "util/util_log.h"

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

using namespace std;

#endif
