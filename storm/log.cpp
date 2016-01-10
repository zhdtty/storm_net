#include "log.h"

#include <stdio.h>
#include <string.h>

const char *briefLogFileName(const char *name) {
	if (name[0] != '/') {
		return name;
	}
	const char* p = strrchr(name, '/');
	if (p != NULL) {
		return p + 1;
	}
	return name;
}

