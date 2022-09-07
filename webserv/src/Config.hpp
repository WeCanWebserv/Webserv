#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "Config.hpp"

struct LocationConfig
{
	std::string root;
	std::vector<std::string> indexFiles;
	std::map<std::string, std::string> tableOfCgiBins;
	std::map<std::string, std::string> tableOfCgiUploads;
	std::pair<int, std::string> redirectionSetting;
	std::set<std::string> allowedMethods;
	bool isAutoIndexOn;
	bool isRedirectionSet;
};

struct ServerConfig
{
	std::vector<std::string> listOfServerNames;
	std::map<int, std::string> tableOfErrorPages;
	std::map<std::string, LocationConfig> tableOfLocations;
	int listennedHost;
	int listennedPort;
	int maxRequestBodySize;
};

struct Config
{
	std::vector<ServerConfig> serverConfigs;
};

#endif // CONFIG_HPP
