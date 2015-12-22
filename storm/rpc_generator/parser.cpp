#include "parser.h"
#include "lex.yy.hpp"
#include "syntax.tab.hpp"

#include "util/util_file.h"
#include "util/util_string.h"

Parser g_parser;

using namespace Storm;
void help() {
	cerr << "Usage generator inputfile\n";
}

void Parser::run(int argc, char** argv) {
	if (argc < 2) {
		help();
		exit(1);
	}
	string fileName = argv[1];
	yyin = fopen(fileName.c_str(), "r");
	if (yyin == NULL) {
		cout << "can't open file " << fileName << endl;
		exit(1);
	}
	yypush_buffer_state(yy_create_buffer(yyin, /*YY_BUF_SIZE*/ 16 * 1024));

	int ret = yyparse();
	if (ret != 0) {
		cout << "error: " << ret << endl;
		exit(1);
	}

	m_fileName = UtilFile::replaceFileExt(UtilFile::getFileBasename(fileName), "", true);

	//show();
	generator();
	cout << "ok!\n" << endl;
}

void Parser::error(const string& err) {
	cerr << "Error: " << yylineno << " : " << err << endl;
	exit(1);
}

void Parser::debug(const string& msg) {
	cout << "Debug " << msg << endl;
}

void Parser::addInclude(const string& file) {
	m_includes.push_back(file);
}

void Parser::addUsingNS(const string& ns) {
	m_usingNameSpace.push_back(ns);
}

void Parser::setNS(const string& ns) {
	m_nameSpace = ns;
}

void Parser::addNewService(const string& service) {
	Service s;
	s.m_serviceName = service;
	m_services.push_back(s);
	m_curService = &m_services[m_services.size() -1];
}

void Parser::addNewFunction(const string& funcName,
						const string& inputClassName, const string& inputParamName,
						const string& outputClassName, const string& outputParamName,
						uint32_t protoId) {
	if (m_curService == NULL) {
		error("curService null");
	}
	Function f;
	f.m_name = funcName;
	f.m_inputClassName = inputClassName;
	f.m_inputParamName = inputParamName;
	f.m_outputClassName = outputClassName;
	f.m_outputParamName = outputParamName;
	f.m_protoId = protoId;
	m_curService->m_functions.push_back(f);
}

void Parser::show() {
	for (uint32_t i = 0; i < m_services.size(); ++i) {
		Service& s = m_services[i];
		cout << "Service: " << s.m_serviceName << endl;
		for (uint32_t j = 0; j < s.m_functions.size(); ++j) {
			Function& f = s.m_functions[j];
			cout << "\t" << f.m_name << endl;
			cout << "\t\t" << f.m_inputClassName
				<< "\t" << f.m_inputParamName
				<< "\t" << f.m_outputClassName
				<< "\t" << f.m_outputParamName
				<< "\t" << f.m_protoId << endl;
		}
	}
}

ostringstream& Parser::tab(int n) {
	for (int i = 0; i < n; ++i) {
		m_oss << "\t";
	}
	return m_oss;
}

void Parser::generator() {
	generatorHead();
	generatorSource();
}

void Parser::generatorHead() {
	m_oss.str("");
	printHead();
	for (uint32_t i = 0; i < m_services.size(); ++i) {
		printServiceHead(m_services[i]);
	}
	printTail();
	
	UtilFile::saveToFile(m_fileName + ".h", m_oss.str());
}

void Parser::generatorSource() {
	m_oss.str("");
	m_oss << "#include \"" << m_fileName << ".h\"" << endl;
	m_oss << endl << endl;
	if (!m_nameSpace.empty()) {
		m_oss << "namespace " << m_nameSpace << " {" << endl;
	}
	for (uint32_t i = 0; i < m_services.size(); ++i) {
		printServiceSource(m_services[i]);
	}
	if (!m_nameSpace.empty()) {
		m_oss << endl;
		m_oss << "}" << endl;
	}

	UtilFile::saveToFile(m_fileName + ".cpp", m_oss.str());
}

void Parser::printHead() {
	m_oss << "#ifndef _STORM_RPC_" << UtilString::toupper(m_fileName) << "_H_" <<  endl;
	m_oss << "#define _STORM_RPC_" << UtilString::toupper(m_fileName) << "_H_" <<  endl;
	m_oss << endl;
	m_oss << "#include \"net/socket_listener.h\"" << endl;
	m_oss << "#include \"net/service_proxy.h\"" << endl;

	m_oss << endl;
	for (uint32_t i = 0; i < m_includes.size(); ++i) {
		string s = m_includes[i];
		m_oss << "#include \"" << UtilFile::replaceFileExt(s, ".pb.h", true) << "\"" << endl;
	}
	m_oss << endl;

	m_oss << "using namespace Storm;" << endl;
	for (uint32_t i = 0; i < m_usingNameSpace.size(); ++i) {
		string s = m_usingNameSpace[i];
		m_oss << "using namespace " << s << ";" << endl;
	}

	m_oss << endl;

	if (!m_nameSpace.empty()) {
		m_oss << "namespace " << m_nameSpace << " {" << endl;
	}

}

void Parser::printTail() {
	if (!m_nameSpace.empty()) {
		m_oss << "}" << endl;
	}
	m_oss << endl;
	m_oss << "#endif" << endl;
}

void Parser::printServiceHead(Service& s) {
	//Service
	m_oss << endl;
	m_oss << "class " << s.m_serviceName << " : public SocketListener {" << endl;
	m_oss << "public:" << endl;
	m_oss << "\t" << "virtual ~" << s.m_serviceName << "(){}" << endl;
	m_oss << endl;
	m_oss << "\t" << "virtual int doRpcRequest(NetPacket::ptr pack, const RpcRequest& req, RpcResponse& resp);" << endl;

	m_oss << endl;
	for (uint32_t i = 0; i < s.m_functions.size(); ++i) {
		Function& f = s.m_functions[i];
		m_oss << "\t" << "virtual void " << f.m_name << "(NetPacket::ptr pack, const "
			<< f.m_inputClassName << "& " << f.m_inputParamName << ", " << f.m_outputClassName
			<< "& " << f.m_outputParamName << ") {}" << endl;
	}
	m_oss << "};" << endl;

	//Proxy
	m_oss << endl;
	m_oss << "class " << s.m_serviceName << "Proxy : public ServiceProxy {" << endl;
	m_oss << "public:" << endl;
	m_oss << "\t" << s.m_serviceName << "Proxy(SocketConnector* connector):ServiceProxy(connector) {}" << endl;
	m_oss << "\t" << "virtual ~" << s.m_serviceName << "Proxy(){}" << endl;

	for (uint32_t i = 0; i < s.m_functions.size(); ++i) {
		m_oss << endl;
		Function& f = s.m_functions[i];
		m_oss << "\t" << "int " << f.m_name << "(const "
			<< f.m_inputClassName << "& " << f.m_inputParamName << ", " << f.m_outputClassName
			<< "& " << f.m_outputParamName << ");" << endl;
		m_oss << "\t" << "void " << "async_" << f.m_name << "(ServiceProxyCallBackPtr cb, const "
			<< f.m_inputClassName << "& " << f.m_inputParamName << ");" << endl;
	}
	m_oss << "};" << endl;


	//CallBack
	m_oss << endl;
	m_oss << "class " << s.m_serviceName << "ProxyCallBack : public ServiceProxyCallBack {" << endl;
	m_oss << "public:" << endl;
	m_oss << "\t" << "virtual ~" << s.m_serviceName << "ProxyCallBack(){}" << endl;
	m_oss << endl;
	m_oss << "\t" << "virtual void dispatch(ReqMessage* req);" << endl;
	m_oss << endl;

	for (uint32_t i = 0; i < s.m_functions.size(); ++i) {
		Function& f = s.m_functions[i];
		m_oss << "\t" << "virtual void callback_" << f.m_name << "(int ret, const "
			<< f.m_outputClassName << "& " << f.m_outputParamName << ") {}" << endl;
	}
	m_oss << "};" << endl;
} 

void Parser::printServiceSource(Service& s) {
	//Service
	m_oss << "int " << s.m_serviceName << "::doRpcRequest(NetPacket::ptr pack, const RpcRequest& req, RpcResponse& resp) {" << endl;
	m_oss << "\t" << "switch (req.proto_id()) {" << endl;
	for (uint32_t i = 0; i < s.m_functions.size(); ++i) {
		Function& f = s.m_functions[i];
		tab(2) << "case " << f.m_protoId << ":" << endl;
		tab(2) << "{" << endl;
		tab(3) << f.m_inputClassName << " request;" << endl;
		tab(3) << f.m_outputClassName << " response;" << endl;
		tab(3) << "if (!request.ParseFromString(req.request())) {" << endl;
		tab(4) << "LOG(\"error\\n\");" << endl;
		tab(4) << "return RespStatus_Coder;" << endl;
		tab(3) << "}" << endl;
		tab(3) << f.m_name << "(pack, request, response);" << endl;
		tab(3) << "if (req.invoke_type() != InvokeType_OneWay) {" << endl;
		tab(4) << "if (!response.SerializeToString(resp.mutable_response())) {" << endl;
		tab(5) << "LOG(\"error\\n\");" << endl;
		tab(5) << "return RespStatus_Coder;" << endl;
		tab(4) << "}" << endl;
		tab(3) << "}" << endl;
		tab(3) << "break;" << endl;
		tab(2) << "}" << endl;
	}
	tab(2) << "default:" << endl;
	tab(3) << "return RespStatus_NoProtoId;" << endl;
	tab(1) << "}" << endl;

	m_oss << endl;
	tab(1) << "return 0;" << endl;
	m_oss << "}" << endl;

	//ServiceProxy
	for (uint32_t i = 0; i < s.m_functions.size(); ++i) {
		m_oss << endl;
		Function& f = s.m_functions[i];

		//同步
		m_oss <<  "int " << s.m_serviceName << "Proxy::" << f.m_name << "(const "
			<< f.m_inputClassName << "& request" << ", " << f.m_outputClassName
			<< "& response) {" << endl;
		tab(1) << "ReqMessage* message = newRequest(InvokeType_Sync);" << endl;
		tab(1) << "message->req.set_proto_id(" << f.m_protoId << ");" << endl;
		tab(1) << "request.SerializeToString(message->req.mutable_request());" << endl;
		m_oss << endl;
		tab(1) << "doInvoke(message);" << endl;
		tab(1) << "int ret = decodeResponse(message, response);" << endl;
		m_oss << endl;
		tab(1) << "delRequest(message);" << endl;
		tab(1) << "return ret;" << endl;
		m_oss << "}" << endl;

		m_oss << endl;

		//异步
		m_oss << "void " << s.m_serviceName << "Proxy::" << "async_" << f.m_name << "(ServiceProxyCallBackPtr cb, const "
			<< f.m_inputClassName << "& request) {" << endl;
		tab(1) << "ReqMessage* message = newRequest(InvokeType_Async, cb);" << endl;
		tab(1) << "uint32_t invokeType = message->invokeType;" << endl;
		tab(1) << "message->req.set_proto_id(" << f.m_protoId << ");" << endl;
		tab(1) << "request.SerializeToString(message->req.mutable_request());" << endl;
		m_oss << endl;
		tab(1) << "doInvoke(message);" << endl;
		m_oss << endl;
		tab(1) << "if (invokeType == InvokeType_OneWay) {" << endl;
		tab(2) << "delRequest(message);" << endl;
		tab(1) << "}" << endl;
		m_oss << "}" << endl;
	}

	//ServiceProxyCallBack
	m_oss << endl;
	m_oss << "void " << s.m_serviceName << "ProxyCallBack::" << "dispatch(ReqMessage* req) {" << endl;
	tab(1) << "uint32_t protoId = req->req.proto_id();" << endl;
	tab(1) << "switch (protoId) {" << endl;
	for (uint32_t i = 0; i < s.m_functions.size(); ++i) {
		Function& f = s.m_functions[i];
		tab(2) << "case " << f.m_protoId << ":" << endl;
		tab(2) << "{" << endl;
		tab(3) << f.m_outputClassName << " response;" << endl;
		tab(3) << "int ret = decodeResponse(req, response);" << endl;
		tab(3) << "callback_" << f.m_name << "(ret, response);" << endl;
		tab(3) << "break;" << endl;
		tab(2) << "}" << endl;
	}
	tab(2) << "default:" << endl;
	tab(2) << "{" << endl;
	tab(3) << "LOG(\"unkown protoId %d\\n\", protoId);" << endl;
	tab(2) << "}" << endl;
	tab(1) << "}" << endl;
	m_oss << "}" << endl;
}





