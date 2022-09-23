#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "Config.hpp"
#include "libft.hpp"

//for test
#include <iostream>

struct LocationBlock
{
	enum kindOfLocationDirectives
	{
		LOC_ROOT,
		LOC_INDEX,
		LOC_AUTOINDEX,
		LOC_REDIRECTION,
		LOC_ALLOWED_METHODS,
		LOC_CGI_BIN
		// LOC_CGI_UPLOAD
	};

	std::set<std::string> availableMethods;
	std::map<std::string, int> kindOf;
	std::vector<std::string> directives;
	bool isInBlock;

	LocationBlock();
	LocationConfig toLocationConfig();
};

struct ServerBlock
{
	enum kindOfServerDirectives
	{
		SERV_NON,
		SERV_SERVER_NAME,
		SERV_ERROR_PAGES,
		SERV_LISTEN,
		SERV_MAX_REQUEST_BODY_SIZE
	};

	std::map<std::string, int> kindOf;
	std::vector<std::string> directives;
	bool isInBlock;

	std::map<std::string, LocationBlock> locationBlocks;
	std::string nextUri;
	LocationBlock nextLocationBlock;

	ServerBlock();
	ServerConfig toServerConfig();
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

	ConfigBlock();
	Config toConfig();
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