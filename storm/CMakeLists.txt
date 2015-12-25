
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_custom_command (
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/proto/rpc_proto.pb.cc
	COMMAND protoc --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/proto/ -I=${CMAKE_CURRENT_SOURCE_DIR}/proto ${CMAKE_CURRENT_SOURCE_DIR}/proto/rpc_proto.proto
	DEPENDS proto/rpc_proto.proto
)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_LIST)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/net SRC_LIST)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/util SRC_LIST)

add_library(storm ${SRC_LIST} ${CMAKE_CURRENT_SOURCE_DIR}/proto/rpc_proto.pb.cc)
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
