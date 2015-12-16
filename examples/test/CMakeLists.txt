set(APP test)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/storm SRC_LIST)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_LIST)
aux_source_directory(${CMAKE_SOURCE_DIR}/storm SRC_LIST)
aux_source_directory(${CMAKE_SOURCE_DIR}/storm/net SRC_LIST)
aux_source_directory(${CMAKE_SOURCE_DIR}/storm/util SRC_LIST)
aux_source_directory(${CMAKE_SOURCE_DIR}/storm/proto SRC_LIST)
aux_source_directory(${CMAKE_SOURCE_DIR}/examples/server_proto SRC_LIST)

add_executable(${APP} ${SRC_LIST})

#find_package(Boost 1.36.0)
#if(Boost_FOUND)
#	message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
#	include_directories(${Boost_INCLUDE_DIRS})
#endif()

target_link_libraries(${APP} pthread protobuf)
