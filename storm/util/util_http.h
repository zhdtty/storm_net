#ifndef _STORM_UTIL_HTTP_H_
#define _STORM_UTIL_HTTP_H_

#include <string>
#include <map>

using namespace std;

namespace Storm {

struct Url {
	Url(): _scheme("http"), _port("80"){}

	string toUrl();
	string getRequest();
	bool parseUrl(const string& url);

	string _scheme;
	string _user;
	string _pass;
	string _host;
	string _port;
	string _path;
	map<string, string> _query;
	string _ref;

	string _url;
};

class HttpBase {
public:

protected:
	map<string, string> m_header;
	string m_content;
};

}

#endif
