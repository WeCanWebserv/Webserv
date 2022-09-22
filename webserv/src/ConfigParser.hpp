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

	LocationBlock() : isInBlock(false)
	{
		std::map<std::string, int> &kindOf = this->kindOf;

		kindOf["root"] = LOC_ROOT;
		kindOf["index"] = LOC_INDEX;
		kindOf["autoindex"] = LOC_AUTOINDEX;
		kindOf["return"] = LOC_REDIRECTION;
		kindOf["allowed_methods"] = LOC_ALLOWED_METHODS;
		kindOf["cgi_bin"] = LOC_CGI_BIN;
		// kindOf["cgi_upload"] = LOC_CGI_UPLOAD;

		availableMethods.insert(std::string("GET"));
		availableMethods.insert(std::string("POST"));
		availableMethods.insert(std::string("DELETE"));
	}

	LocationConfig toLocationConfig()
	{
		LocationConfig locationConfig;
		for (size_t i = 0; i < directives.size(); i++)
		{
			std::vector<std::string> tokens = ft_split(directives[i], ' ');
			// const std::string &directive = tokens.front();
			const std::string &directive = tokens[0];
			if (this->kindOf.find(directive) == this->kindOf.end())
				throw std::runtime_error("invalid location block directive");
			switch (this->kindOf[directive])
			{
			case LOC_ROOT:
			{
				if (tokens.size() != 2)
					throw std::runtime_error("invalid location block: root:");
				locationConfig.root = tokens[1];
				break;
			}
			case LOC_INDEX:
			{
				if (tokens.size() < 2)
					throw std::runtime_error("invalid location block: root:");
				if (!locationConfig.indexFiles.empty())
					throw std::runtime_error("invalid location block: index: duplicated");
				locationConfig.indexFiles.insert(locationConfig.indexFiles.begin(), ++tokens.begin(),
																				 tokens.end());
				break;
			}
			case LOC_AUTOINDEX:
			{
				if (tokens.size() != 2 || (tokens[1] != "on" && tokens[1] != "off"))
					throw std::runtime_error("invalid location block: autoindex");
				locationConfig.isAutoIndexOn = tokens[1] == "on" ? true : false;
				break;
			}
			case LOC_REDIRECTION:
			{
				if (tokens.size() < 2 || tokens.size() > 3)
					throw std::runtime_error("invalid location block: redirection");
				std::stringstream buffer(tokens[1]);
				int statusCode;
				buffer >> statusCode;
				if (!buffer.eof())
					throw std::runtime_error("invalid location block: return:");
				locationConfig.redirectionSetting.first = statusCode;
				if (tokens.size() == 3)
					locationConfig.redirectionSetting.second = tokens[2];
				locationConfig.isRedirectionSet = true;
				break;
			}
			case LOC_ALLOWED_METHODS:
			{
				if (tokens.size() < 2)
					throw std::runtime_error("invalid location block: allowed_method:");
				for (size_t i = 1; i < tokens.size(); i++)
				{
					if (this->availableMethods.find(tokens[i]) == this->availableMethods.end())
						throw std::runtime_error("invalid location block: allowed_method:");
					if (locationConfig.allowedMethods.insert(tokens[i]).second == false)
						throw std::runtime_error("invalid location block: allowed_method:");
				}
				break;
			}
			case LOC_CGI_BIN:
			{
				if (tokens.size() != 3)
					throw std::runtime_error("invalid location block: cgi_bin:");
				if (tokens[1][0] != '.')
					// if (tokens[1].front() != '.')
					throw std::runtime_error("invalid location block: cgi_bin:");
				if (locationConfig.tableOfCgiBins.insert(std::make_pair(tokens[1], tokens[2])).second ==
						false)
					throw std::runtime_error("invalid location block: cgi_bin: duplicated");
				break;
			}
			case LOC_CGI_UPLOAD:
			{
				if (tokens.size() != 3)
					throw std::runtime_error("invalid location block: cgi_upload:");
				if (tokens[1][0] != '.')
					// if (tokens[1].front() != '.')
					throw std::runtime_error("invalid location block: cgi_upload:");
				if (locationConfig.tableOfCgiBins.find(tokens[1]) == locationConfig.tableOfCgiBins.end())
					throw std::runtime_error("invalid location block: cgi_upload:");
				if (locationConfig.tableOfCgiUploads.insert(std::make_pair(tokens[1], tokens[2])).second ==
						false)
					throw std::runtime_error("invalid location block: cgi_upload: duplicated");
			}
			}
		}
		return locationConfig;
	}
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

	ServerBlock() : isInBlock(false)
	{
		kindOf["server_name"] = SERV_SERVER_NAME;
		kindOf["error_page"] = SERV_ERROR_PAGES;
		kindOf["listen"] = SERV_LISTEN;
		kindOf["max_request_body_size"] = SERV_MAX_REQUEST_BODY_SIZE;
	}
	ServerConfig toServerConfig()
	{
		ServerConfig serverConfig;
		for (size_t i = 0; i < this->directives.size(); i++)
		{
			std::vector<std::string> tokens = ft_split(directives[i], ' ');
			// const std::string &directive = tokens.front();
			const std::string &directive = tokens[0];
			if (this->kindOf.find(directive) == this->kindOf.end())
				throw std::runtime_error("invalid server block directive");
			switch (this->kindOf[directive])
			{
			case SERV_SERVER_NAME:
			{
				if (tokens.size() < 2)
					throw std::runtime_error("invalied server block: server_name");
				if (!serverConfig.listOfServerNames.empty())
					throw std::runtime_error("invalied server block: server_name");
				serverConfig.listOfServerNames.insert(serverConfig.listOfServerNames.begin(),
																							++tokens.begin(), tokens.end());
				break;
			}
			case SERV_ERROR_PAGES:
			{
				if (tokens.size() < 3)
					throw std::runtime_error("invalied server block: error_page");
				const std::string &page = tokens.back();
				for (size_t i = 1; i < tokens.size() - 1; i++)
				{
					std::stringstream buffer(tokens[i]);
					int statusCode;
					buffer >> statusCode;
					if (serverConfig.tableOfErrorPages.insert(std::make_pair(statusCode, page)).second ==
							false)
						throw std::runtime_error("invalied server block: error_page");
				}
				break;
			}
			case SERV_LISTEN:
			{
				if (tokens.size() != 3)
					throw std::runtime_error("invalied server block: listen");
				// 유효한 아이피 어드레스 포맷, 유효한 정수인지 확인이 필요할 수 있음.
				serverConfig.listennedHost = tokens[1];
				serverConfig.listennedPort = tokens[2];
				break;
			}
			case SERV_MAX_REQUEST_BODY_SIZE:
			{
				std::stringstream buffer(tokens[1]);
				int maxReqSize;
				buffer >> maxReqSize;
				if (!buffer.eof())
					throw std::runtime_error("invalied server block: max_request_body_size");
				serverConfig.maxRequestBodySize = maxReqSize;
				break;
			}
			default: // empty...
				break;
			}
			for (std::map<std::string, LocationBlock>::iterator it = this->locationBlocks.begin();
					 it != this->locationBlocks.end(); it++)
			{
				serverConfig.tableOfLocations.insert(
						std::make_pair(it->first, it->second.toLocationConfig()));
			}
		}
		return serverConfig;
	};
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

	ConfigBlock() : isInBlock(false), context(CONTEXT_NON) {}
	Config toConfig()
	{
		Config config;
		for (std::vector<ServerBlock>::iterator it = this->serverBlocks.begin();
				 it != this->serverBlocks.end(); it++)
		{
			config.serverConfigs.push_back(it->toServerConfig());
		}
		return config;
	}
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