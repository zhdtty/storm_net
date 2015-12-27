
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${PROTOBUF_INCLUDE_DIRS})

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS proto/rpc_proto.proto)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_LIST)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/net SRC_LIST)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/util SRC_LIST)

add_library(storm ${SRC_LIST} ${PROTO_SRCS} ${PROTO_HDRS})

install(TARGETS storm DESTINATION lib)

file(GLOB headers1 *.h)
install(FILES ${headers1} DESTINATION include)

file(GLOB headers2 net/*.h)
install(FILES ${headers2} DESTINATION include/net)

file(GLOB headers3 util/*.h)
install(FILES ${headers3} DESTINATION include/util)

file(GLOB headers4 proto/*.h)
install(FILES ${headers4} DESTINATION include/proto)

add_subdirectory(rpc_generator)
