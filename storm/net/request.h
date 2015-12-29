#ifndef _NET_REQUEST_H_
#define _NET_REQUEST_H_

#include "common_header.h"
#include "proto/rpc_proto.pb.h"

namespace Storm {

class ServiceProxy;
class ServiceProxyCallBack;
class RecvPacketHandler;
typedef std::shared_ptr<ServiceProxyCallBack> ServiceProxyCallBackPtr;

enum InvokeType {
	InvokeType_Sync,
	InvokeType_Async,
	InvokeType_OneWay,
};

enum RespStatus {
	RespStatus_Ok = 0, 		//正常返回
	RespStatus_TimeOut = 1, //调用超时
	RespStatus_UnReachableHost = 2, //目标不能连接
	RespStatus_CoderError = 3,   //编解码出错
	RespStatus_NoProtoId = 4, //协议不存在
	RespStatus_Error = 5, //未知错误
};

struct ReqMessage {
	ReqMessage():invokeType(InvokeType_Sync), sockId(0), status(RespStatus_TimeOut), cb(NULL), resp(NULL), handler(NULL) {}

	uint32_t invokeType;
	uint32_t sockId;
	RespStatus status;
	ServiceProxyCallBackPtr cb;

	RpcRequest req;
	RpcResponse* resp;

	RecvPacketHandler* handler;
};

}

#endif
