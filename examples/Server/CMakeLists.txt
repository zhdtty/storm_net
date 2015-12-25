set(APP Server)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/storm)
include_directories(${CMAKE_SOURCE_DIR}/examples/RpcProto)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_LIST)

add_executable(${APP} ${SRC_LIST}  $<TARGET_OBJECTS:${RPC_TARGET}> )

target_link_libraries(${APP} pthread protobuf storm)
