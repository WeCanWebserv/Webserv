#ifndef CONFIGINFO_HPP
#define CONFIGINFO_HPP

#include <map>
#include <string>
#include <vector>

#include "Location.hpp"

class ConfigInfo
{
// private:
public:
	std::string serverName;
	std::string host; // default: required
	int port;
	std::map<int, std::string> errorPages;     // defautl: empty
	std::map<std::string, Location> locations; // required
	int status;                                // END, BAD ... operator ! 연산 구현

public:
	ConfigInfo() {}
	std::string getHost() const
	{
		return this->host;
	}
	int getPort() const
	{
		return this->port;
	}
	std::string &getServerName()
	{
		return this->serverName;
	}
};

#endif // CONFIGINFO_HPP
