#include "GameService.h"

#include <unistd.h>

#include "util/util_time.h"

//测试用
uint64_t g_time = 0;
uint32_t g_count = 0;

void GameService::doClose(NetPacket::ptr pack) {
	LOG("doClose id %d fd %d\n", pack->id, pack->fd);
}

//TODO 返回错误码应该是业务层的
int GameService::doRpcRequest(NetPacket::ptr pack, const RpcRequest& req, RpcResponse& resp) {
	/*
	LOG("proto id %d\n", req.proto_id());
	LOG("requestId %d\n", req.request_id());
	LOG("invokeType %d\n", req.invoke_type());
	*/

	switch (req.proto_id()) {
		case 1:
		{
			EchoRequest request;
			EchoResponse response;
			if (!request.ParseFromString(req.request())) {
				LOG("error\n");
				return RespStatus_Coder;
			}
			Echo(pack, request, response);
			if (req.invoke_type() != InvokeType_OneWay) {
				if (!response.SerializeToString(resp.mutable_response())) {
					LOG("error\n");
					return RespStatus_Coder;
				}
			}
			break;
		}
		default:
			return RespStatus_NoProtoId;
			break;
	}

	//统计
	g_count++;
	uint64_t t = UtilTime::getNowMS();
	if (t > (g_time + 3 * 1000)) {
		uint32_t count = g_count  * 1000 / (t - g_time);
		g_time  = t;
		LOG("tqs %d\n", count);
		showLen();
		g_count = 0;
	}
	return 0;
}

int GameService::Echo(NetPacket::ptr pack, const EchoRequest& request, EchoResponse& resp) {
	//LOG("echo %s\n", request.msg().c_str());
	resp.set_msg(request.msg());
	return 0;
}
