#include "protocol.h"

#include "util/util_log.h"
#include "util/http_parser.h"

namespace Storm {

static int on_info(http_parser* p) {
//	LOG_DEBUG << "on info";
	return 0;
}

static int on_data(http_parser* p, const char *at, size_t length) {
//	LOG_DEBUG << "on data";
	return 0;
}

int FrameProtocolHttp::decode(IOBuffer::ptr in, string& out) {
	if (in->getSize() == 0) {
		return Packet_Less;
	}
	/*
	http_parser_settings settings;
	memset(&settings, 0, sizeof(settings));
	settings.on_message_begin = on_info,
	settings.on_headers_complete = on_info,
	settings.on_message_complete = on_info,
	settings.on_header_field = on_data,
	settings.on_header_value = on_data,
	settings.on_url = on_data,
	settings.on_status = on_data,
	settings.on_body = on_data;

	struct http_parser parser;
	http_parser_init(&parser, HTTP_REQUEST);
	size_t n = http_parser_execute(&parser, &settings, in->getHead(), in->getSize());
	*/

	out.assign(in->getHead(), in->getSize());
	in->readN(in->getSize());
	return Packet_Normal;
	/*
	uint32_t len = in->getSize();
	if (len < 4) {
		return Packet_Less;
	}
	char* data = in->getHead();
	uint32_t packetLen = 0;
	for (uint32_t i = 0; i < 4; i++) {
		uint32_t shift_bit = (3-i)*8;
		packetLen += (data[i]<<shift_bit)&(0xFF<<shift_bit);
	}
	if (len < packetLen) {
		return Packet_Less;
	}
	out.assign(in->getHead() + 4, packetLen - 4);
	in->readN(packetLen);

	return Packet_Normal;
	*/
}

}

