#include "ConfigParser.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "libft.hpp"

//
// public:
//
ConfigParser::ConfigParser(const char *path) : configFile(path)
{
	if (!this->configFile)
		throw std::runtime_error("file not found");
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

//
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
			throw std::runtime_error("syntax wrong: context location");

		serverBlock.isInBlock = true;
	}
	else if (token == "}")
	{
		if (!serverBlock.isInBlock)
			throw std::runtime_error("syntax wrong: context location");

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
			throw std::runtime_error("syntax wrong: context location");

		if (token == "location")
		{
			std::string uri;
			std::getline(lineBuffer, uri, ' ');
			// if (uri.empty() || uri.front() != '/')
			if (uri.empty() || uri[0] != '/')
				throw std::runtime_error("syntax wrong: context location");

			std::map<std::string, LocationBlock> &locationBlocks = serverBlock.locationBlocks;
			if (locationBlocks.find(uri) != locationBlocks.end())
				throw std::runtime_error("duplicated url");

			serverBlock.nextUri = uri;
			configBlock.context = CONTEXT_LOCATION;
		}
		else
		{
			if (line[line.length() - 1] != ';')
				throw std::runtime_error("syntax wrong: context location");

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
			throw std::runtime_error("syntax wrong: context location");

		locationBlock.isInBlock = true;
	}
	else if (token == "}")
	{
		if (!locationBlock.isInBlock)
			throw std::runtime_error("syntax wrong: context location");

		serverBlock.locationBlocks.insert(std::make_pair(nextUri, locationBlock));
		serverBlock.nextLocationBlock.directives.clear();
		locationBlock.isInBlock = false;
		configBlock.context = CONTEXT_SERVER;
	}
	else
	{
		if (!locationBlock.isInBlock)
			throw std::runtime_error("syntax wrong: context location");

		if (line[line.length() - 1] != ';')
			throw std::runtime_error("syntax wrong: context location");

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
		throw std::runtime_error("syntax wrong: context non");
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
		// TOOD
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