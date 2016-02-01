#include "util_http.h"

#include <string.h>
#include "util_string.h"

namespace Storm {

string Url::toUrl() {
	_url.clear();

	_url = _scheme;
	_url += "://";

	if (!_user.empty() && !_pass.empty()) {
		_url += _user;
		_url += ":";
		_url += _pass;
		_url += "@";
	}

	_url += _host;
	if (_port != "80") {
		_url += _port;
	}


    _url += getRequest();

	return _url;
}

string Url::getRequest() {
	string url;
	if (!_path.empty()) {
		url += _path;
	}
	if (!_query.empty()) {
		url += "?";
		url += UtilString::joinURLParam(_query);
	}
	if (!_ref.empty()) {
		url += "#" + _ref;
	}

	return url;
}

bool Url::parseUrl(const string& url) {
	string request = UtilString::trim(url);

	if (request.empty()) {
		return false;
	}
    //clear();

	int pos = 0;
	if (strncasecmp(request.c_str(), "http://" ,7) == 0) {
		pos = 7;
		_scheme = "http";
	}
	string::size_type index = request.find("/", pos);
	string::size_type queryIndex = request.find("?", pos);
	string::size_type refIndex = request.find("#", pos);

	string urlAuthority;
	string urlPath;
	if (queryIndex < index) {
		urlAuthority = request.substr(pos, queryIndex - pos);
		urlPath = "/" + request.substr(queryIndex);
	} else if (refIndex < index) {
		urlAuthority = request.substr(pos, refIndex - pos);
		urlPath = "/" + request.substr(refIndex); 
	} else {
		if (index == string::npos) {
			urlAuthority = request.substr(pos, index);
			urlPath = "";
		} else {
			urlAuthority = request.substr(pos, index - pos);
			urlPath = request.substr(index);
		}
	}

	//user pass和host
	index = urlAuthority.find("@");
	if (index != string::npos) {
		_user = urlAuthority.substr(0, index);
		_host = urlAuthority.substr(index + 1);
	} else {
		_host = urlAuthority;
	}
	
	index = _user.find(":");
	if (index != string::npos) {
		_pass = _user.substr(index + 1);
		_user = _user.substr(0, index);
	}

	//host port
	index = _host.find(":");
	if (index != string::npos) {
		_port = _host.substr(index + 1);
		_host = _host.substr(0, index);
	}

	index = urlPath.find("?");
	string query;
	if (index != string::npos) {
		_path = urlPath.substr(0, index);
		query = urlPath.substr(index + 1);

		index = query.rfind("#");
		if (index != string::npos) {
			_ref = query.substr(index + 1);
			query = query.substr(0, index);
		}
	} else {
		_path = urlPath;
		query = "";

		index = _path.rfind("#");
		if (index != string::npos) {
			_ref = _path.substr(index + 1);
			_path = _path.substr(0, index);
		}
	}

	if (_path.empty()) {
		_path = "/";
	}

	if (_host.find(".") == string::npos) {
		return false;
	}
	UtilString::splitURLParam(query, _query);

	toUrl();

	return true;
}
}
