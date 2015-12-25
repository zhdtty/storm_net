set(APP Client)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/storm)
include_directories(${CMAKE_SOURCE_DIR}/examples/RpcProto)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_LIST)

add_executable(${APP} ${SRC_LIST}  $<TARGET_OBJECTS:${RPC_TARGET}> )

target_link_libraries(${APP} pthread protobuf storm)

#find_package(Boost 1.36.0)
#if(Boost_FOUND)
#	message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
#	include_directories(${Boost_INCLUDE_DIRS})
#endif()
