#ifndef _MFW_UTIL_OPTION_
#define _MFW_UTIL_OPTION_

#include <map>
#include <vector>
#include <string>
using namespace std;

namespace Storm
{

class COption
{
public:
	COption():m_stop(false),m_configFile("app.conf") {}

    void parse(int argc, char *argv[]);
	string getConfigFile() { return m_configFile; }
	bool isStop() { return m_stop; }
	void displayUsage(const string& name);

private:
	bool m_stop;
	string m_configFile;
};

}

#endif

