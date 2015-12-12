#include "util_option.h"
#include "util_string.h"

#include <unistd.h>
#include <getopt.h>

#include <iostream>

using namespace std;

namespace Storm
{

struct option opts[] = {
	{"help", no_argument, NULL, 'h'},
	{"stop", no_argument, NULL, 's'},
	{"config", required_argument, NULL, 'f'}
};

const char* option_str = "hsf:";

void COption::parse(int argc, char *argv[]) {
	int opt = 0;
	while ((opt = getopt_long(argc, argv, option_str, opts, NULL)) != -1) {
		switch (opt) {
			case 'h':
				displayUsage(argv[0]);
				exit(0);
				break;
			case 's':
				m_stop = true;
				break;
			case 'f':
				m_configFile = optarg;
				break;
			case '?':
				//cout << "unknown" << endl;
				break;
			default:
				displayUsage(argv[0]);
				exit(0);
		}
	}
}

void COption::displayUsage(const string& name) {
	cout << "Usage " << name << " [-f config]|[-h]|[-s]" << endl << endl;
	cout << "	-f --config	config file, default app.conf" << endl;
	cout << "	-h --help	help" << endl;
	cout << "	-s --stop	stop" << endl;
}

}
