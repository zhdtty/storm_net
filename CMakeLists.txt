cmake_minimum_required(VERSION 2.8)
project(Hope)


#路径设置
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR})

#安装路径
set(CMAKE_INSTALL_PREFIX /usr/local/storm/)
message(STATUS ${CMAKE_INSTALL_PREFIX})

#编译选项设置
set(CMAKE_BUILD_TYPE Debug)
#set (CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -std=c++11 -O2 -Wall -g -ggdb")
#set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
#set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -std=c++11 -O0 -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -std=c++11 -O2 -Wall")


#依赖包
find_package(Protobuf REQUIRED)


add_subdirectory(storm)
add_subdirectory(examples EXCLUDE_FROM_ALL)
