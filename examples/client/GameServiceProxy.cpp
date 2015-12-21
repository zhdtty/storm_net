#include "GameServiceProxy.h"

int GameServiceProxy::Echo(const EchoRequest& request, EchoResponse& response) {
	ReqMessage* message = newRequest(InvokeType_Sync);
	message->req.set_proto_id(1);
	request.SerializeToString(message->req.mutable_request());

	doInvoke(message);
	int ret = decodeResponse(message, response);

	delRequest(message);
	return ret;
}

void GameServiceProxy::async_Echo(ServiceProxyCallBackPtr cb, const EchoRequest& request) {
	ReqMessage* message = newRequest(InvokeType_Async, cb);
	uint32_t invokeType = message->invokeType;
	message->req.set_proto_id(1);
	request.SerializeToString(message->req.mutable_request());

	doInvoke(message);

	//单向调用此时可以删掉请求，之后用不到了
	if (invokeType == InvokeType_OneWay) {
		delete message;
	}
}

void GameServiceProxyCallBack::dispatch(ReqMessage* req) {
	uint32_t protoId = req->req.proto_id();

	switch (protoId) {
		case 1:
		{
			EchoResponse response;
			int ret = decodeResponse(req, response);
			callback_Echo(ret, response);
			break;
		}
		default:
		{
			LOG("unkown protoId %d\n", protoId);
		}
	}
	//delRequest(req);
}

