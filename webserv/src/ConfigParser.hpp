#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

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

#endif // CONFIGPARSER_HPP