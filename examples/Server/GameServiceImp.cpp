#include "GameServiceImp.h"

#include <unistd.h>
#include "util/util_time.h"

//测试用
uint64_t g_time = 0;
uint32_t g_count = 0;

void GameServiceImp::doClose(NetPacket::ptr pack) {
	LOG("doClose id %d fd %d\n", pack->id, pack->fd);
}

void GameServiceImp::Echo(NetPacket::ptr pack, const EchoRequest& request, EchoResponse& resp) {
	resp.set_msg(request.msg());

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
}
