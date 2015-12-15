#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include "common_header.h"
#include "io_buffer.h"

namespace Storm {

enum {
	Packet_Normal,
	Packet_Less,
	Packet_Error,
};

class FrameProtocolLen {
public:
	static IOBuffer::ptr encode(const string& data) {
		uint32_t packetLen = data.size() + 4;
		IOBuffer::ptr packet(new IOBuffer(packetLen));
		packet->push_back(packetLen);
		packet->push_back(data);
		return packet;
	}

	template <typename T>
	static IOBuffer::ptr encode(const T& pb) {
		uint32_t pbLen = pb.ByteSize();
		uint32_t packetLen = pbLen + 4;
		IOBuffer::ptr packet(new IOBuffer(packetLen));
		packet->push_back(packetLen);
		if (!pb.SerializeToArray(packet->getHead() + 4, pbLen)) {
			return IOBuffer::ptr();
		}
		packet->writeN(pbLen);
		return packet;
	}

	static IOBuffer::ptr encode(IOBuffer::ptr buffer) {
		uint32_t packetLen =  buffer->getSize() + 4;
		IOBuffer::ptr packet(new IOBuffer(packetLen));
		packet->push_back(packetLen);
		packet->push_back(buffer);
		return packet;
	}

	template <int maxLen = 65535>
	static int decode(IOBuffer::ptr in, string& out) {
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
		if (packetLen > maxLen) {
			return Packet_Error;
		}
		if (len < packetLen) {
			return Packet_Less;
		}
		out.assign(in->getHead() + 4, packetLen - 4);
		in->readN(packetLen);

		return Packet_Normal;
	}
};

}

#endif
