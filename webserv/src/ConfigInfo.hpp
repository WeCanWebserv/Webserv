#ifndef CONFIGINFO_HPP
#define CONFIGINFO_HPP

#include <map>
#include <string>
#include <vector>

#include "Location.hpp"

class ConfigInfo
{
private:
	int host;                                  // default: required
	std::vector<int> ports;                    // default: required
	std::map<int, std::string> errorPages;     // defautl: empty
	std::map<std::string, Location> locations; // required
	int status;                                // END, BAD ... operator ! 연산 구현

public:
	ConfigInfo() {}
	int getHost() const
	{
		return this->host;
	}
	const std::vector<int> &getPorts() const
	{
		return this->ports;
	}
};

#endif // CONFIGINFO_HPP
