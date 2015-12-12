#ifndef _NET_REQUEST_H_
#define _NET_REQUEST_H_

#include "common_header.h"
#include "proto/rpc_proto.pb.h"

namespace Storm {

class ServiceProxy;
class ServiceProxyCallBack;
class RecvPacketHandler;
typedef boost::shared_ptr<ServiceProxyCallBack> ServiceProxyCallBackPtr;

enum InvokeType {
	InvokeType_Sync,
	InvokeType_Async,
	InvokeType_OneWay,
};

enum RespStatus {
	RespStatus_Ok = 0, 		//正常返回
	RespStatus_TimeOut = 1, //调用超时
	RespStatus_UnReachableHost = 2, //目标不能连接
	RespStatus_Coder = 3,   //编解码出错
	RespStatus_Error = 4, //未知错误
};

struct ReqMessage {
	ReqMessage():invokeType(InvokeType_Sync), status(RespStatus_TimeOut), resp(NULL), proxy(NULL), handler(NULL) {}

	uint32_t invokeType;
	RespStatus status;
	ServiceProxyCallBackPtr cb;

	RpcRequest req;
	RpcResponse* resp;

	ServiceProxy* proxy;
	RecvPacketHandler* handler;
};

}

#endif
