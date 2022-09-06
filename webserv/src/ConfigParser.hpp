#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "Config.hpp"

struct LocationBlock
{
	std::vector<std::string> directives;
	bool isInBlock;

	LocationBlock() : isInBlock(false) {}
};

struct ServerBlock
{
	std::vector<std::string> directives;
	bool isInBlock;

	std::map<std::string, LocationBlock> locationBlocks;
	std::string nextUri;
	LocationBlock nextLocationBlock;

	ServerBlock() : isInBlock(false) {}
};

class ConfigContext
{
public:
	enum eConfigContext
	{
		CONTEXT_NON,
		CONTEXT_SERVER,
		CONTEXT_LOCATION
	};
};

struct ConfigBlock : public ConfigContext
{
	std::vector<std::string> directives;
	bool isInBlock;

	std::vector<ServerBlock> serverBlocks;
	ServerBlock nextServerBlock;
	int context;

	ConfigBlock() : context(CONTEXT_NON), isInBlock(false) {}
};

class ConfigParser : public ConfigContext
{
private:
	std::ifstream configFile;
	Config config;

public:
	ConfigParser(const char *path);
	~ConfigParser();

	const Config &getConfig() const;

protected:
	void populate();

	void fillBufferExceptComment(std::ifstream &inputStream,
															 std::stringstream &bufferStream,
															 char commentChar = '#');

	void modifyBufferAroundBraket(std::stringstream &bufferStream);
	void modifyBufferAroundLineBreak(std::stringstream &bufferStream);

	void reconstructBufferToLines(std::vector<std::string> &lines, std::stringstream &bufferStream);

	void handleLineInServerBlock(ConfigBlock &configBlock, std::string &line);
	void handleLineInLocationBlock(ConfigBlock &configBlock, std::string &line);
	void handleLineInNoContext(ConfigBlock &configBlock, std::string &line);

private:
	ConfigParser();
	ConfigParser(const ConfigParser &other);
	ConfigParser &operator=(const ConfigParser &other);
};

#endif // CONFIGPARSER_HPP

// void populate()
// {
// 	std::vector<ServerConfig> &serverConfigs = this->config.serverConfigs;

// 	ServerConfig server;
// 	{
// 		server.listennedPort = 8888;
// 		server.listennedHost = INADDR_ANY;

// 		server.listOfServerNames.push_back("example.com");
// 		server.listOfServerNames.push_back("www.example.com");

// 		server.maxRequestBodySize = 10240;
// 		server.tableOfErrorPages.insert(std::make_pair(501, "/error/50x.html"));
// 		server.tableOfErrorPages.insert(std::make_pair(502, "/error/50x.html"));

// 		LocationConfig &location = server.tableOfLocations["/"];
// 		{
// 			location.root = "/usr/src/webserv";

// 			location.indexFiles.push_back("index.html");
// 			location.indexFiles.push_back("index.htm");

// 			location.tableOfCgiBins.insert(std::make_pair(".php", "/usr/local/opt/php"));
// 			location.tableOfCgiBins.insert(std::make_pair(".py", "/usr/local/opt/python@3.10"));

// 			location.tableOfCgiUploads.insert(std::make_pair(".py", "/usr/local/upload"));
// 		}
// 	}
// 	serverConfigs.push_back(server);
// }
