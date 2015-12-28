
#set(CMAKE_INCLUDE_CURRENT_DIR on)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${PROTOBUF_INCLUDE_DIRS})

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_LIST)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/net SRC_LIST)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/util SRC_LIST)

set(PROTO_META_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto)
list(APPEND PROTO_FLAGS -I${PROTO_META_BASE_DIR})

file(GLOB RPC_PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)

foreach(FIL ${RPC_PROTOS})
	get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
	get_filename_component(FIL_WE ${FIL} NAME_WE)

	list(APPEND PROTO_SRCS ${PROTO_META_BASE_DIR}/${FIL_WE}.pb.cc)
	list(APPEND PROTO_HDRS ${PROTO_META_BASE_DIR}/${FIL_WE}.pb.h)

	execute_process(
		COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} ${PROTO_FLAGS} --cpp_out=${PROTO_META_BASE_DIR} ${ABS_FIL}
	)
endforeach()


#PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS proto/rpc_proto.proto)

add_library(storm ${SRC_LIST} ${PROTO_SRCS} ${PROTO_HDRS})

install(TARGETS storm DESTINATION lib)

file(GLOB headers1 *.h)
file(GLOB headers2 net/*.h)
file(GLOB headers3 util/*.h)
file(GLOB headers4 proto/*.h)

install(FILES ${headers1} DESTINATION include)
install(FILES ${headers2} DESTINATION include/net)
install(FILES ${headers3} DESTINATION include/util)
install(FILES ${headers4} DESTINATION include/proto)

add_subdirectory(rpc_generator)
