find_path(MYSQLCLIENT_INCLUDE_DIR mysql.h /usr/include
         /usr/local/mysql/include)
find_library(MYSQLCLIENT_LIBRARY NAMES mysqlclient PATH /usr/lib
         /usr/local/mysql/lib)
if (MYSQLCLIENT_INCLUDE_DIR AND MYSQLCLIENT_LIBRARY)
    set(MYSQLCLIENT_FOUND TRUE)
endif (MYSQLCLIENT_INCLUDE_DIR AND MYSQLCLIENT_LIBRARY)

if (MYSQLCLIENT_FOUND)
	if (NOT MYSQLCLIENT_FIND_QUIETLY)
		message(STATUS "Found MYSQLCLIENT: ${MYSQLCLIENT_LIBRARY}")
	endif (NOT MYSQLCLIENT_FIND_QUIETLY)
else (MYSQLCLIENT_FOUND)
    if (MYSQLCLIENT_FIND_REQUIRED)
        message(FATAL_ERROR "Could not find mysqlclient library")
    endif(MYSQLCLIENT_FIND_REQUIRED)
endif(MYSQLCLIENT_FOUND)

