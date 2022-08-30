// #include "ServerManager.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <stdexcept>
#include <vector>

void ft_sed3(std::stringstream &strStream, const std::string &s1, const std::string &s2)
{
	if (s1.empty() || s1 == s2)
	{
		return;
	}

	std::stringstream sedStream;
	while (1)
	{
		std::string replacedLine;
		std::string currentLine;
		std::getline(strStream, currentLine);
		size_t currentIdx = 0;
		while (1)
		{
			size_t foundIdx = currentLine.find(s1, currentIdx);
			if (foundIdx == std::string::npos)
			{
				replacedLine += currentLine.substr(currentIdx, currentLine.length() - currentIdx);
				break;
			}
			if (currentIdx < foundIdx)
			{
				replacedLine += currentLine.substr(currentIdx, foundIdx - currentIdx);
				currentIdx = foundIdx;
			}
			replacedLine += s2;
			currentIdx += s1.length();
		}

		sedStream << replacedLine;
		if (strStream.eof())
		{
			break;
		}

		sedStream << '\n';
	}
	strStream.str(sedStream.str());
	strStream.clear();
}

template<class Directive>
class setting
{
	std::pair<std::string, Directive> setting;
};

enum ConfigContext
{
	CONTEXT_NON,
	CONTEXT_HTTP,
	CONTEXT_SERVER,
	CONTEXT_LOCATION
};

struct location
{
	location() : directives(), isInBlock(false) {}
	std::vector<std::string> directives;
	bool isInBlock;
	void clear()
	{
		this->isInBlock = false;
		this->directives.clear();
	}
};

struct server
{
	server() : directives(), isInBlock(false) {}
	std::vector<std::string> directives;
	std::map<std::string, location> locations;
	location nextLocation;
	std::string nextKey;
	bool isInBlock;
	void clear()
	{
		this->directives.clear();
		this->locations.clear();
		this->isInBlock = false;
		this->nextLocation.clear();
		this->nextKey.clear();
	}
};

struct http
{
	http() : directives(), servers(), isInBlock(false), nextId(0) {}
	std::vector<std::string> directives;
	std::map<int, server> servers;
	struct server nextServer;
	int nextId;
	bool isInBlock;
};

struct parser
{
	parser() : directives(), http(), context(CONTEXT_NON), isInBlock(false) {}
	std::vector<std::string> directives;
	struct http http;
	int context;
	bool isInBlock;
};

void ft_trim(std::string& str, const std::string& ws = "\t\n\v\f\r ")
{
	size_t firstIdx = str.find_first_not_of(ws);
	if (firstIdx == std::string::npos)
		return;
	else
	{
		size_t lastIdx = str.find_last_not_of(ws);
		str.substr(firstIdx, lastIdx);
	}

}

void print(struct parser &p)
{
	std::cout << "@ Global" << std::endl;
	std::vector<std::string> &globalDirectives = p.directives;
	for (int i = 0; i < globalDirectives.size(); i++)
	{
		std::cout << '\t' << globalDirectives[i] << std::endl;
	}

	std::cout << std::endl;

	std::cout << "@ Http" << std::endl;
	std::vector<std::string> &httpDirectives = p.http.directives;
	for (int i = 0; i < httpDirectives.size(); i++)
	{
		std::cout << '\t' << httpDirectives[i] << std::endl;
	}
	std::cout << std::endl;

	std::map<int, struct server> &servers = p.http.servers;
	for (std::map<int, struct server>::iterator first = servers.begin(); first != servers.end();
			 first++)
	{
		int id = first->first;
		struct server &serv = first->second;
		std::vector<std::string> directives = serv.directives;
		std::cout << "\t@ Servers[" << id << "]" << std::endl;
		for (int i = 0; i < directives.size(); i++)
		{
			std::cout << "\t\t" << directives[i] << std::endl;
		}
		std::cout << std::endl;
		for (std::map<std::string, struct location>::iterator first = serv.locations.begin();
				 first != serv.locations.end(); first++)
		{
			const std::string &path = first->first;
			std::vector<std::string> &directives = first->second.directives;
			std::cout << "\t\t@ location [" << path << "]" << std::endl;
			for (int i = 0; i < directives.size(); i++)
			{
				std::cout << "\t\t\t" << directives[i] << std::endl;
			}
		}
	}
}

int main()
{
	std::ifstream configFile("../config/default.conf");
	if (!configFile)
	{
		std::cerr << "file not found" << std::endl;
		exit(1);
	}
	std::stringstream configBuffer;
	std::string line;
	while (!configFile.eof())
	{
		std::getline(configFile, line);

		size_t foundIdx = line.find('#');
		if (foundIdx == 0 || line.empty())
			continue;

		if (foundIdx != std::string::npos)
		{
			line = line.substr(0, foundIdx - 1);
		}
		configBuffer << line;
	}
	configFile.close();

	std::cout << configBuffer.str() << std::endl;
	ft_sed3(configBuffer, "{", "\n{\n");
	ft_sed3(configBuffer, ";", ";\n");
	ft_sed3(configBuffer, "}", "\n}\n");

	std::vector<std::string> lines;
	while (!configBuffer.eof())
	{
		std::string line;
		std::getline(configBuffer, line);
		ft_trim(line);
		if (line.empty())
		{
			continue;
		}
		lines.push_back(line);
	}
	struct parser parser;

	std::string token;
	std::stringstream lineBuffer;
	for (int i = 0; i < lines.size(); i++)
	{
		lineBuffer.str(lines[i]);
		std::getline(lineBuffer, token, ' ');
		switch (parser.context)
		{
		case CONTEXT_HTTP:
		{
			struct http &http = parser.http;
			if (token == "{")
			{
				if (http.isInBlock)
				{
					std::cout << "line: " << __LINE__ << std::endl;
					print(parser);
					throw std::runtime_error("syntax wrong: context http");
				}
				http.isInBlock = true;
			}
			else if (token == "}")
			{
				if (!http.isInBlock)
				{
					std::cout << "line: " << __LINE__ << std::endl;
					print(parser);
					throw std::runtime_error("syntax wrong: context http");
				}
				http.isInBlock = false;
				parser.context = CONTEXT_NON;
			}
			else
			{
				if (!http.isInBlock)
				{
					std::cout << "line: " << __LINE__ << std::endl;
					print(parser);
					throw std::runtime_error("syntax wrong: context http");
				}
				if (token == "server")
				{
					parser.context = CONTEXT_SERVER;
					break;
				}
				if (lines[i].back() != ';')
				{
					std::cout << "line: " << __LINE__ << std::endl;
					print(parser);
					throw std::runtime_error("syntax wrong: context http");
				}
				else
				{
					http.directives.push_back(lines[i].substr(0, lines[i].length() - 1));
				}
			}
			break;
		}
		case CONTEXT_SERVER:
		{
			struct server &serv = parser.http.nextServer;
			if (token == "{")
			{
				if (serv.isInBlock)
				{
					std::cout << "line: " << __LINE__ << std::endl;
					print(parser);
					throw std::runtime_error("syntax wrong: context location");
				}
				serv.isInBlock = true;
			}
			else if (token == "}")
			{
				if (!serv.isInBlock)
				{
					std::cout << "line: " << __LINE__ << std::endl;
					print(parser);
					throw std::runtime_error("syntax wrong: context location");
				}
				parser.http.servers.insert(std::make_pair(parser.http.nextId++, parser.http.nextServer));
				parser.http.nextServer.clear();
				serv.isInBlock = false;
				parser.context = CONTEXT_HTTP;
			}
			else
			{
				if (!serv.isInBlock)
				{
					std::cout << "line: " << __LINE__ << std::endl;
					print(parser);
					throw std::runtime_error("syntax wrong: context location");
				}
				if (token == "location")
				{
					std::getline(lineBuffer, token, ' ');
					if (token.empty() || token.front() != '/')
					{
						std::cout << "line: " << __LINE__ << std::endl;
						print(parser);
						throw std::runtime_error("syntax wrong: context location");
					}
					if (serv.locations.find(token) != serv.locations.end())
					{
						throw std::runtime_error("duplicated url");
					}
					serv.nextKey = token;
					parser.context = CONTEXT_LOCATION;
					break;
				}
				if (lines[i].back() != ';')
				{
					std::cout << "line: " << __LINE__ << std::endl;
					print(parser);
					throw std::runtime_error("syntax wrong: context location");
				}
				else
				{
					serv.directives.push_back(lines[i].substr(0, lines[i].length() - 1));
				}
			}
			break;
		}
		case CONTEXT_LOCATION:
		{
			struct server &serv = parser.http.nextServer;
			struct location &location = serv.nextLocation;
			if (token == "{")
			{
				if (location.isInBlock)
				{
					std::cout << "line: " << __LINE__ << std::endl;
					print(parser);
					throw std::runtime_error("syntax wrong: context location");
				}
				location.isInBlock = true;
			}
			else if (token == "}")
			{
				if (!location.isInBlock)
				{
					std::cout << "line: " << __LINE__ << std::endl;
					print(parser);
					throw std::runtime_error("syntax wrong: context location");
				}
				location.isInBlock = false;

				serv.locations.insert(std::make_pair(serv.nextKey, serv.nextLocation));
				serv.nextKey.clear();
				serv.nextLocation.clear();
				parser.context = CONTEXT_SERVER;
			}
			else
			{
				if (!location.isInBlock)
				{
					std::cout << "line: " << __LINE__ << std::endl;
					print(parser);
					throw std::runtime_error("syntax wrong: context location");
				}
				if (lines[i].back() != ';')
				{
					std::cout << "line: " << __LINE__ << std::endl;
					print(parser);
					throw std::runtime_error("syntax wrong: context location");
				}
				else
				{
					location.directives.push_back(lines[i].substr(0, lines[i].length() - 1));
				}
			}
			break;
		}
		default:
		{
			if (token == "http")
			{
				parser.context = CONTEXT_HTTP;
			}
			else if (lines[i].back() == ';')
			{
				parser.directives.push_back(lines[i].substr(0, lines[i].length() - 1));
			}
			else
			{
				std::cout << "token: " << token << std::endl;
				std::cout << "line: " << __LINE__ << std::endl;
				print(parser);
				throw std::runtime_error("syntax wrong: context non");
			}
			break;
		}
		}
		lineBuffer.clear();
	}
	print(parser);
}
