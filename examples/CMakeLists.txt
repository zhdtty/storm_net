set(RPC_TARGET whatever)

include_directories(${CMAKE_SOURCE_DIR}/storm)

add_custom_command (
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/RpcProto/GameService.pb.cc ${CMAKE_CURRENT_SOURCE_DIR}/RpcProto/GameService.pb.h
	COMMAND protoc --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/RpcProto -I=${CMAKE_CURRENT_SOURCE_DIR}/RpcProto ${CMAKE_CURRENT_SOURCE_DIR}/RpcProto/GameService.proto
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/RpcProto/GameService.proto
)

add_custom_command (
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/RpcProto/GameService.cpp ${CMAKE_CURRENT_SOURCE_DIR}/RpcProto/GameService.h
	COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/RpcProto && ${PROJECT_BINARY_DIR}/rpc2cpp GameService.rpc
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/RpcProto/GameService.rpc
)

set(RPC_SRC ${CMAKE_CURRENT_SOURCE_DIR}/RpcProto/GameService.pb.cc ${CMAKE_CURRENT_SOURCE_DIR}/RpcProto/GameService.cpp)

add_library(${RPC_TARGET} OBJECT ${RPC_SRC})


add_subdirectory(Server)
add_subdirectory(Client)
