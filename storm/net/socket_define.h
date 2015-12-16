#ifndef _SOCKET_DEFINE_H_
#define _SOCKET_DEFINE_H_

#include "common_header.h"

namespace Storm {

typedef std::function<int (IOBuffer::ptr, string&)> ProtocolType;

class SocketListener;
class SocketClient;
class SocketHandler;

enum Socket_Type {
	Socket_Type_Listen,		//监听
	Socket_Type_FromOut,	//外部连接进来
	Socket_Type_ToOut,		//主动连接外部
	Socket_Type_Notify,		//激活epoll用的
};

enum Socket_Status {
	Socket_Status_Idle,
	Socket_Status_Reserve,		//保留准备使用
	Socket_Status_Listen,		//监听中
	Socket_Status_Connecting,	//连接中
	Socket_Status_Connected, 	//已连接
};

enum Socket_CloseType {
	CloseType_Server = 0,	//服务器主动断开
	CloseType_Client = 1,   //客户端主动断开
	CloseType_Timeout = 2,  //超时
	CloseType_EmptyTimeout = 3, //空连接超时
	CloseType_Packet_Error = 4, //协议包出错
	CloseType_ConnectFail = 5, //连接失败
};

enum Socket_Cmd_Type {
	Socket_Cmd_Connect,
	Socket_Cmd_Listen,
	Socket_Cmd_Send,
	Socket_Cmd_Close,
	Socket_Cmd_Exit,
};

struct SocketCmd {
	SocketCmd(): type(Socket_Cmd_Listen), id(0), closeType(CloseType_Server) {}
	int type;
	int	id;
	int closeType;
	IOBuffer::ptr buffer;
};

struct Socket {
	Socket():id(0), fd(-1), status(Socket_Status_Idle), handler(NULL), client(NULL) {}
	int id;
	int fd;
	int type;
	int status;
	SocketHandler* handler;
	SocketClient* client;
	string ip;
	int port;

	std::list<IOBuffer::ptr> writeBuffer;
	IOBuffer::ptr	readBuffer;
};

enum NetPacket_Type {
	Net_Close = 0,
	Net_Packet = 1,
};

}

#endif
