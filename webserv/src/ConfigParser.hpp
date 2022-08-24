#include <sstream>

#include "ConfigInfo.hpp"

class ConfigParser
{
private:
	const char *path;
	std::stringstream strBuf;

public:
	ConfigParser(const char *path) : path(path) {}

	ConfigInfo getInfo()
	{
		return ConfigInfo();
	}
	bool remain()
	{
		return false;
	}
};