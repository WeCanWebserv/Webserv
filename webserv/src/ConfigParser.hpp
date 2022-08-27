#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include "ConfigInfo.hpp"
#include "Location.hpp"

#include <iostream>
#include <stdexcept>

class ConfigParser
{
private:
	const char *path;
	std::stringstream strBuf;

	int __getTokenType(const std::string &tokenStr)
	{
		if (tokenStr == "server_name")
			return CONF_SERVER_NAME;

		else if (tokenStr == "host")
			return CONF_HOST;

		else if (tokenStr == "port")
			return CONF_PORT;

		else if (tokenStr == "error_page")
			return CONF_ERROR_PAGE;

		else if (tokenStr == "location")
			return CONF_LOCATION;

		else
			return CONF_INVALID;
	}

public:
	enum ConfigType
	{
		CONF_SERVER_NAME,
		CONF_HOST,
		CONF_PORT,
		CONF_ERROR_PAGE,
		CONF_LOCATION,
		CONF_INVALID
	};

	ConfigParser(const char *path) : path(path), strBuf(path)
	{
		if (!strBuf)
		{
			std::cerr << "file not found\n" << std::endl;
		}
	}

	Location getLocation(std::stringstream &strBuf)
	{
		Location location;
		std::string token;

		strBuf >> location.path;
		strBuf >> token;
		while (!token.empty() && token != "}")
		{
			if (token == "root")
			{
				strBuf >> location.root;
			}
			else if (token == "allow_method")
			{
				strBuf >> token;
				location.allowedMethods[token] = true;
			}
			else if (token == "index")
			{
				strBuf >> token;
			}
			else if (token == "auto_index")
			{
				strBuf >> token;
				if (token == "on")
					location.autoIndexOn = true;
				else if (token == "off")
					location.autoIndexOn = false;
				else
				{
					// error
				}
			}
			else if (token == "cgi_info")
			{
				strBuf >> location.cgiInfo.extension;
				strBuf >> location.cgiInfo.binPath;
				strBuf >> location.cgiInfo.uploadPath;
			}
			// else if (token == "return")
			// {}
			else
			{
				// empty
			}
		}

		return location;
	}

	ConfigInfo getInfo()
	{
		std::string token;

		strBuf >> token;
		if (token != "server")
			throw std::runtime_error("syntax wrong");
		strBuf >> token;
		if (token != "{")
			throw std::runtime_error("syntax wrong");

		strBuf >> token;
		ConfigInfo info;
		while (!token.empty() && token != "}")
		{
			switch (this->__getTokenType(token))
			{
			case CONF_SERVER_NAME:
			{
				strBuf >> info.serverName;
				break;
			}
			case CONF_HOST:
			{
				strBuf >> info.host;
				break;
			}
			case CONF_PORT:
			{
				strBuf >> info.port;
				break;
			}
			case CONF_ERROR_PAGE:
			{
				int errorType;
				std::string errorPagePath;
				strBuf >> errorType;
				strBuf >> errorPagePath;
				info.errorPages.insert(make_pair(errorType, errorPagePath));
				break;
			}
			case CONF_LOCATION:
			{
				strBuf >> token;
				if (token != "{")
					throw std::runtime_error("syntax wrong");
				Location location = this->getLocation(strBuf);
				break;
			}
			default:
				break;
			}
		}

		return ConfigInfo();
	}

	bool remain()
	{
		return false;
	}
};

#endif // CONFIGPARSER_HPP