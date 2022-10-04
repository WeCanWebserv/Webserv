#include "ConfigParser.hpp"

#include <errno.h>
#include <string.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "Logger.hpp"
#include "libft.hpp"

//
// class ConfigParser
//

// public:
//
ConfigParser::ConfigParser(const char *path) : configFile(path)
{
	if (!this->configFile)
	{
		Logger::error() << __func__ << ":" << __LINE__ << " configuration file not found" << std::endl;
		throw std::runtime_error(strerror(ENOENT));
	}
	this->populate();
}

const Config &ConfigParser::getConfig() const
{
	return this->config;
}

ConfigParser::~ConfigParser()
{
	this->configFile.close();
}

// protected:
//
void ConfigParser::fillBufferExceptComment(std::ifstream &inputStream,
																					 std::stringstream &bufferStream,
																					 char commentChar)
{
	std::string line;
	while (!inputStream.eof())
	{
		std::getline(inputStream, line);
		size_t foundIdx = line.find(commentChar);
		if (foundIdx == 0 || line.empty())
			continue;

		if (foundIdx != std::string::npos)
			line = line.substr(0, foundIdx - 1);

		bufferStream << line;
	}
	bufferStream.clear();
}

void ConfigParser::modifyBufferAroundBraket(std::stringstream &bufferStream)
{
	ft_replace(bufferStream, "{", "\n{\n");
	ft_replace(bufferStream, "}", "\n}\n");
}

void ConfigParser::modifyBufferAroundLineBreak(std::stringstream &bufferStream)
{
	ft_replace(bufferStream, ";", ";\n");
}

void ConfigParser::reconstructBufferToLines(std::vector<std::string> &lines,
																						std::stringstream &bufferStream)
{
	while (!bufferStream.eof())
	{
		std::string line;
		std::getline(bufferStream, line);
		ft_trim(line);
		if (line.empty())
			continue;

		lines.push_back(line);
	}

	bufferStream.clear();
}

void ConfigParser::handleLineInServerBlock(ConfigBlock &configBlock, std::string &line)
{
	std::stringstream lineBuffer;
	std::string token;

	lineBuffer.str(line);
	std::getline(lineBuffer, token, ' ');
	ServerBlock &serverBlock = configBlock.nextServerBlock;
	if (token == "{")
	{
		if (serverBlock.isInBlock)
		{
			Logger::error() << __func__ << ":" << __LINE__ << "syntax error: unexpected token '{'"
											<< std::endl;
			throw std::runtime_error("syntax error: unexpected token '{'");
		}

		serverBlock.isInBlock = true;
	}
	else if (token == "}")
	{
		if (!serverBlock.isInBlock)
		{
			Logger::error() << __func__ << ":" << __LINE__ << "syntax error: unexpected token '}'"
											<< std::endl;
			throw std::runtime_error("syntax error: unexpected token '}'");
		}

		configBlock.serverBlocks.push_back(serverBlock);
		configBlock.nextServerBlock.directives.clear();
		configBlock.nextServerBlock.isInBlock = false;
		configBlock.nextServerBlock.nextUri.clear();
		configBlock.nextServerBlock.locationBlocks.clear();
		configBlock.context = CONTEXT_NON;
	}
	else
	{
		if (!serverBlock.isInBlock)
		{
			Logger::error() << __func__ << ":" << __LINE__ << "syntax error: token '{' expected"
											<< std::endl;
			throw std::runtime_error("syntax error: token '{' expected");
		}

		if (token == "location")
		{
			std::string uri;
			std::getline(lineBuffer, uri, ' ');
			if (uri.empty() || uri[0] != '/')
			{
				Logger::error() << __func__ << ":" << __LINE__ << " LocationBlock: invalid path"
												<< std::endl;
				throw std::runtime_error("syntax wrong: context location");
			}

			std::map<std::string, LocationBlock> &locationBlocks = serverBlock.locationBlocks;
			if (locationBlocks.find(uri) != locationBlocks.end())
			{
				Logger::error() << __func__ << ":" << __LINE__ << " LocationBlock: duplicated url"
												<< std::endl;
				throw std::runtime_error("duplicated url");
			}

			serverBlock.nextUri = uri;
			configBlock.context = CONTEXT_LOCATION;
		}
		else
		{
			if (line[line.length() - 1] != ';')
			{
				Logger::error() << __func__ << ":" << __LINE__
												<< "syntax error: linebreak ';' not found in directives" << std::endl;
				throw std::runtime_error("syntax error: ';' not found in directives");
			}

			serverBlock.directives.push_back(line.substr(0, line.length() - 1));
		}
	}
}

void ConfigParser::handleLineInLocationBlock(ConfigBlock &configBlock, std::string &line)
{
	std::stringstream lineBuffer;
	std::string token;

	lineBuffer.str(line);
	std::getline(lineBuffer, token, ' ');

	ServerBlock &serverBlock = configBlock.nextServerBlock;
	std::string &nextUri = serverBlock.nextUri;
	LocationBlock &locationBlock = serverBlock.nextLocationBlock;
	if (token == "{")
	{
		if (locationBlock.isInBlock)
		{
			Logger::error() << __func__ << ":" << __LINE__ << "syntax error: unexpected token '{'"
											<< std::endl;
			throw std::runtime_error("syntax error: unexpected token '{'");
		}

		locationBlock.isInBlock = true;
	}
	else if (token == "}")
	{
		if (!locationBlock.isInBlock)
		{
			Logger::error() << __func__ << ":" << __LINE__ << "syntax error: unexpected token '}'"
											<< std::endl;
			throw std::runtime_error("syntax error: unexpected token '}'");
		}

		serverBlock.locationBlocks.insert(std::make_pair(nextUri, locationBlock));
		serverBlock.nextLocationBlock.directives.clear();
		locationBlock.isInBlock = false;
		configBlock.context = CONTEXT_SERVER;
	}
	else
	{
		if (!locationBlock.isInBlock)
		{
			Logger::error() << __func__ << ":" << __LINE__ << "syntax error: token '{' expected"
											<< std::endl;
			throw std::runtime_error("syntax error: token '{' expected");
		}

		if (line[line.length() - 1] != ';')
		{
			Logger::error() << __func__ << ":" << __LINE__
											<< "syntax error: linebreak ';' not found in directives" << std::endl;
			throw std::runtime_error("syntax error: ';' not found in directives");
		}

		locationBlock.directives.push_back(line.substr(0, line.length() - 1));
	}
}

void ConfigParser::handleLineInNoContext(ConfigBlock &configBlock, std::string &line)
{
	std::stringstream lineBuffer;
	std::string token;

	lineBuffer.str(line);
	std::getline(lineBuffer, token, ' ');

	if (token == "server")
		configBlock.context = CONTEXT_SERVER;
	else if (line[line.length() - 1] != ';')
	{
		Logger::error() << __func__ << ":" << __LINE__ << " configuration file not found" << std::endl;
		throw std::runtime_error("syntax wrong: context non");
	}
	else
		configBlock.directives.push_back(line.substr(0, line.length() - 1));
}

void ConfigParser::populate()
{
	std::stringstream configBuffer;

	this->fillBufferExceptComment(this->configFile, configBuffer, '#');
	this->modifyBufferAroundBraket(configBuffer);
	this->modifyBufferAroundLineBreak(configBuffer);

	std::vector<std::string> lines;
	this->reconstructBufferToLines(lines, configBuffer);

	ConfigBlock configBlock;
	for (size_t i = 0; i < lines.size(); i++)
	{
		std::string &line = lines[i];
		switch (configBlock.context)
		{
		case CONTEXT_SERVER:
			handleLineInServerBlock(configBlock, line);
			break;
		case CONTEXT_LOCATION:
			handleLineInLocationBlock(configBlock, line);
			break;
		default:
			handleLineInNoContext(configBlock, line);
			break;
		}
	}
	this->config = configBlock.toConfig();
}

//
// struct LocationBlock
//
LocationBlock::LocationBlock() : isInBlock(false)
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

LocationConfig LocationBlock::toLocationConfig()
{
	LocationConfig locationConfig;
	bool isAutoIndexInitialized = false;

	locationConfig.isRedirectionSet = false;
	for (size_t i = 0; i < directives.size(); i++)
	{
		std::vector<std::string> tokens = ft_split(directives[i], ' ');
		const std::string &directive = tokens[0];
		if (this->kindOf.find(directive) == this->kindOf.end())
		{
			Logger::error() << __func__ << ":" << __LINE__ << " LocationBlock: invalid directive"
											<< std::endl;
			throw std::runtime_error("invalid directive");
		}
		switch (this->kindOf[directive])
		{
		case LOC_ROOT:
		{
			if (!locationConfig.root.empty())
			{
				Logger::error() << __func__ << ":" << __LINE__ << " LocationBlock: root already initialized"
												<< std::endl;
				throw std::runtime_error("root already initialized");
			}
			if (tokens.size() != 2)
			{
				Logger::error() << __func__ << ":" << __LINE__
												<< " LocationBlock: root: invalid number of directive arguments."
												<< std::endl;
				throw std::runtime_error("invalid number of directive arguments.");
			}
			if (*tokens[1].rbegin() != '/')
				tokens[1].push_back('/');
			locationConfig.root = tokens[1];
			break;
		}
		case LOC_INDEX:
		{
			if (tokens.size() < 2)
			{
				Logger::error() << __func__ << ":" << __LINE__
												<< " LocationBlock: index: invalid number of directive arguments."
												<< std::endl;
				throw std::runtime_error("invalid number of directive arguments.");
			}
			if (!locationConfig.indexFiles.empty())
			{
				Logger::error() << __func__ << ":" << __LINE__
												<< " LocationBlock: index: index files data already initialized"
												<< std::endl;
				throw std::runtime_error("index files data already initialized");
			}
			locationConfig.indexFiles.insert(locationConfig.indexFiles.begin(), ++tokens.begin(),
																			 tokens.end());
			break;
		}
		case LOC_AUTOINDEX:
		{
			if (isAutoIndexInitialized)
			{
				Logger::error() << __func__ << ":" << __LINE__
												<< " LocationBlock: autoindex already initialized" << std::endl;
				throw std::runtime_error("autoindex already initialized");
			}
			if (tokens.size() != 2 || (tokens[1] != "on" && tokens[1] != "off"))
			{
				Logger::error() << __func__ << ":" << __LINE__
												<< " LocationBlock: autoindex: invalid directive arguments" << std::endl;
				throw std::runtime_error("invalid directive arguments");
			}
			locationConfig.isAutoIndexOn = tokens[1] == "on" ? true : false;
			isAutoIndexInitialized = true;
			break;
		}
		case LOC_REDIRECTION:
		{
			if (locationConfig.isRedirectionSet)
			{
				Logger::error() << __func__ << ":" << __LINE__
												<< " LocationBlock: return: redirection data already initialized"
												<< std::endl;
				throw std::runtime_error("redirection data already initialized");
			}
			if (tokens.size() < 2 || tokens.size() > 3)
			{
				Logger::error() << __func__ << ":" << __LINE__
												<< " LocationBlock: index: invalid number of directive arguments."
												<< std::endl;
				throw std::runtime_error("invalid number of directive arguments.");
			}
			std::stringstream buffer(tokens[1]);
			int statusCode;
			buffer >> statusCode;
			if (!buffer.eof())
			{
				Logger::error() << __func__ << ":" << __LINE__
												<< " LocationBlock: redirection: invalid integer string format"
												<< std::endl;
				throw std::runtime_error("invaild integer string format");
			}
			locationConfig.redirectionSetting.first = statusCode;
			if (tokens.size() == 3)
				locationConfig.redirectionSetting.second = tokens[2];
			locationConfig.isRedirectionSet = true;
			break;
		}
		case LOC_ALLOWED_METHODS:
		{
			if (tokens.size() < 2)
			{
				Logger::error() << __func__ << ":" << __LINE__
												<< " LocationBlock: index: invalid number of directive arguments."
												<< std::endl;
				throw std::runtime_error("invalid number of directive arguments.");
			}
			for (size_t i = 1; i < tokens.size(); i++)
			{
				if (this->availableMethods.find(tokens[i]) == this->availableMethods.end())
				{
					Logger::error() << __func__ << ":" << __LINE__ << "LocationBlock: unavailable methods"
													<< std::endl;
					throw std::runtime_error("unavailable methods");
				}
				if (locationConfig.allowedMethods.insert(tokens[i]).second == false)
				{
					Logger::error() << __func__ << ":" << __LINE__ << "LocationBlock: duplicated methods"
													<< std::endl;
					throw std::runtime_error("duplicated methods");
				}
			}
			break;
		}
		case LOC_CGI_BIN:
		{
			if (tokens.size() != 3)
			{
				Logger::error() << __func__ << ":" << __LINE__
												<< " LocationBlock: index: invalid number of directive arguments."
												<< std::endl;
				throw std::runtime_error("invalid number of directive arguments.");
			}
			if (tokens[1][0] != '.')
			{
				Logger::error() << __func__ << ":" << __LINE__ << " LocationBlock: invaild cgi-extension"
												<< std::endl;
				throw std::runtime_error("invalid cgi-extension");
			}
			if (locationConfig.tableOfCgiBins.insert(std::make_pair(tokens[1], tokens[2])).second ==
					false)
			{
				Logger::error() << __func__ << ":" << __LINE__
												<< " LocationBlock: duplicated cgi_bin extension" << std::endl;
				throw std::runtime_error("duplicated cgi_bin extension");
			}
			break;
		}
		}
	}

	if (locationConfig.allowedMethods.empty())
		locationConfig.allowedMethods.insert("GET");

	return locationConfig;
}

//
// struct ServerBlock
//
ServerBlock::ServerBlock() : isInBlock(false)
{
	kindOf["server_name"] = SERV_SERVER_NAME;
	kindOf["error_page"] = SERV_ERROR_PAGES;
	kindOf["listen"] = SERV_LISTEN;
	kindOf["max_request_body_size"] = SERV_MAX_REQUEST_BODY_SIZE;
}

ServerConfig ServerBlock::toServerConfig()
{
	ServerConfig serverConfig;
	bool isBodySizeInitialized = false;

	for (size_t i = 0; i < this->directives.size(); i++)
	{
		std::vector<std::string> tokens = ft_split(directives[i], ' ');
		const std::string &directive = tokens[0];
		if (this->kindOf.find(directive) == this->kindOf.end())
		{
			Logger::error() << __func__ << ":" << __LINE__ << " ServerBlock: invalid directive"
											<< std::endl;
			throw std::runtime_error("invalid directive");
		}
		switch (this->kindOf[directive])
		{
		case SERV_SERVER_NAME:
		{
			if (tokens.size() < 2)
			{
				Logger::error() << __func__ << ":" << __LINE__
												<< " ServerBlock: server_name: invalid number of directive arguments."
												<< std::endl;
				throw std::runtime_error("invalid number of directive arguments.");
			}
			if (!serverConfig.listOfServerNames.empty())
			{
				Logger::error() << __func__ << ":" << __LINE__
												<< " ServerBlock: server_name already initialized" << std::endl;
				throw std::runtime_error("server_name already initialized");
			}
			serverConfig.listOfServerNames.insert(serverConfig.listOfServerNames.begin(),
																						++tokens.begin(), tokens.end());
			break;
		}
		case SERV_ERROR_PAGES:
		{
			if (tokens.size() < 3)
			{
				Logger::error() << __func__ << ":" << __LINE__
												<< " ServerBlock: error_pages: invalid number of directive arguments."
												<< std::endl;
				throw std::runtime_error("invalid number of directive arguments.");
			}
			const std::string &page = tokens.back();
			for (size_t i = 1; i < tokens.size() - 1; i++)
			{
				std::stringstream buffer(tokens[i]);
				int statusCode;
				buffer >> statusCode;
				if (serverConfig.tableOfErrorPages.insert(std::make_pair(statusCode, page)).second == false)
				{
					Logger::error() << __func__ << ":" << __LINE__ << " ServerBlock: duplicated error_pages"
													<< std::endl;
					throw std::runtime_error("duplicated error_pages");
				}
			}
			break;
		}
		case SERV_LISTEN:
		{
			if (!serverConfig.listennedHost.empty() || !serverConfig.listennedPort.empty())
			{
				Logger::error() << __func__ << ":" << __LINE__
												<< " ServerBlock: listenned host and port already initialized" << std::endl;
				throw std::runtime_error("listenned host and port already initialized");
			}
			if (tokens.size() != 3)
			{
				Logger::error() << __func__ << ":" << __LINE__
												<< " ServerBlock: listen: invalid number of directive arguments."
												<< std::endl;
				throw std::runtime_error("invalid number of directive arguments.");
			}
			std::stringstream buffer;
			// buffer.str(tokens[1]);
			// int ipAddress;
			// buffer >> ipAddress;
			// if (!buffer.eof())
			// {
			// 	Logger::error() << __func__ << ":" << __LINE__
			// 									<< " ServerBlock: configuration file not found" << std::endl;
			// 	throw std::runtime_error("invalied server block: listen");
			// }
			serverConfig.listennedHost = tokens[1];

			buffer.clear();
			int port;
			buffer.str(tokens[2]);
			buffer >> port;
			if (!buffer.eof() || port < 0)
			{
				Logger::error() << __func__ << ":" << __LINE__ << " ServerBlock: port: format error"
												<< std::endl;
				throw std::runtime_error("format error: invaild port");
			}
			serverConfig.listennedPort = tokens[2];
			break;
		}
		case SERV_MAX_REQUEST_BODY_SIZE:
		{
			if (isBodySizeInitialized)
			{
				Logger::error() << __func__ << ":" << __LINE__
												<< " ServerBlock: max_request_body_size already initialized" << std::endl;
				throw std::runtime_error("max_request_body_size already initialized");
			}
			std::stringstream buffer(tokens[1]);
			int maxReqSize;
			buffer >> maxReqSize;
			if (!buffer.eof())
			{
				Logger::error() << __func__ << ":" << __LINE__ << "max_requst_body_size: format error"
												<< std::endl;
				throw std::runtime_error("format error: invaild integer string");
			}
			serverConfig.maxRequestBodySize = maxReqSize;
			isBodySizeInitialized = true;
			break;
		}
		default:
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

//
// struct ConfigBlock
//
ConfigBlock::ConfigBlock() : isInBlock(false), context(CONTEXT_NON) {}

Config ConfigBlock::toConfig()
{
	Config config;
	for (std::vector<ServerBlock>::iterator it = this->serverBlocks.begin();
			 it != this->serverBlocks.end(); it++)
	{
		config.serverConfigs.push_back(it->toServerConfig());
	}
	return config;
}