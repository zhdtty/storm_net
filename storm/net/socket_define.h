#ifndef _SOCKET_DEFINE_H_
#define _SOCKET_DEFINE_H_

#include "common_header.h"

namespace Storm {

typedef boost::function<int (IOBuffer::ptr, string&)> ProtocolType;

class SocketListener;
class SocketClient;

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

enum Socket_Close_Type {
	Server_Close,
	Client_Close,
	Timeout_Close,
	Connect_Error,
};

enum Socket_Cmd_Type {
	Socket_Cmd_Connect,
	Socket_Cmd_Listen,
	Socket_Cmd_Send,
	Socket_Cmd_Close,
	Socket_Cmd_Exit,
};

struct SocketCmd {
	SocketCmd(): type(Socket_Cmd_Listen), id(0) {}
	int type;
	int	id;
	IOBuffer::ptr buffer;
};

struct Socket {
	Socket():id(0), fd(-1), status(Socket_Status_Idle), listener(NULL), client(NULL) {}
	int id;
	int fd;
	int type;
	int status;
	SocketListener* listener;
	SocketClient* client;
	string ip;
	int port;

	std::list<IOBuffer::ptr> writeBuffer;
	IOBuffer::ptr	readBuffer;
};

}

#endif
