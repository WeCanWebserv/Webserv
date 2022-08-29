// #include "ServerManager.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <stdexcept>
#include <vector>

using std::string;

// int ft_sed(const string &filename, const string &s1, const string &s2)
// {
// 	// setIFS

// 	if (filename.empty() == true)
// 	{
// 		std::cout << "Error: filename is empty\n";
// 		return 1;
// 	}

// 	if (s1.empty() == true)
// 	{
// 		std::cout << "Error: string1 is empty\n";
// 		return 1;
// 	}

// 	std::ifstream ifs(filename);
// 	if (ifs.is_open() == false)
// 	{
// 		std::cout << "Error\n";
// 		std::perror(filename.c_str());
// 		return 1;
// 	}

// 	// setOFS
// 	std::ofstream ofs(filename + ".replace");
// 	if (ofs.is_open() == false)
// 	{
// 		std::cout << "Error\n";
// 		std::perror("ofstream");
// 		ifs.close();
// 		return 1;
// 	}

// 	if (s1 == s2)
// 	{
// 		// createCopyNoReplaced
// 		while (1)
// 		{
// 			string line;
// 			std::getline(ifs, line);
// 			ofs << line << '\n';
// 			if (ifs.eof() == true)
// 			{
// 				break;
// 			}
// 		}
// 	}
// 	else
// 	{
// 		// createCopyReplaced
// 		while (1)
// 		{
// 			string replaced_line;
// 			string origin_line;
// 			std::getline(ifs, origin_line);
// 			size_t pos = 0;
// 			while (1)
// 			{
// 				size_t found_index = origin_line.find(s1, pos);
// 				if (found_index == string::npos)
// 				{
// 					replaced_line += origin_line.substr(pos, origin_line.length() - pos);
// 					break;
// 				}
// 				if (pos < found_index)
// 				{
// 					replaced_line += origin_line.substr(pos, found_index - pos);
// 					pos = found_index;
// 				}
// 				replaced_line += s2;
// 				pos += s1.length();
// 			}
// 			ofs << replaced_line;
// 			if (ifs.eof() == true)
// 			{
// 				break;
// 			}
// 			ofs << '\n';
// 		}
// 	}
// 	// ~Sed
// 	ifs.close();
// 	ofs.close();
// 	return 0;
// }

std::stringstream ft_sed2(std::ifstream &inputStream, const string &s1, const string &s2)
{
	if (s1.empty() || s1 == s2)
	{
		return std::stringstream();
	}

	std::stringstream sedStream;
	while (1)
	{
		string replacedLine;
		string currentLine;
		std::getline(inputStream, currentLine);
		size_t currentIdx = 0;
		while (1)
		{
			size_t foundIdx = currentLine.find(s1, currentIdx);
			if (foundIdx == string::npos)
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
		if (inputStream.eof() == true)
		{
			break;
		}

		sedStream << '\n';
	}
	return sedStream;
}

void ft_sed3(std::stringstream &strStream, const string &s1, const string &s2)
{
	if (s1.empty() || s1 == s2)
	{
		return;
	}

	std::stringstream sedStream;
	while (1)
	{
		string replacedLine;
		string currentLine;
		std::getline(strStream, currentLine);
		size_t currentIdx = 0;
		while (1)
		{
			size_t foundIdx = currentLine.find(s1, currentIdx);
			if (foundIdx == string::npos)
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

std::vector<std::string> gDirectives;

struct location
{
	location() : url(), directives(), isInBlock(false) {}
	std::string url;
	std::vector<std::string> directives;
	bool isInBlock;
};
struct server
{
	server() : directives(), loc(), isInBlock(false) {}
	std::vector<std::string> directives;
	location loc;
	bool isInBlock;
};

struct http
{
	http() : directives(), serv(), isInBlock(false) {}
	std::vector<std::string> directives;
	struct server serv;
	bool isInBlock;
};

struct parser
{
	parser() : directives(),http(), context(CONTEXT_NON), isInBlock(false) {}
	std::vector<std::string> directives;
	struct http http;
	int context;
	bool isInBlock;
};

void print(struct parser &p)
{
	std::vector<std::string> &globalDirectives = p.directives;
	std::vector<std::string> &httpDirectives = p.http.directives;
	std::vector<std::string> &serverDirectives = p.http.serv.directives;
	std::vector<std::string> &locationDirectives = p.http.serv.loc.directives;
	std::cout << "@ Global" << std::endl;
	for (int i = 0; i < globalDirectives.size(); i++)
	{
		std::cout << globalDirectives[i] << std::endl;
	}

	std::cout << "@ Http" << std::endl;
	for (int i = 0; i < httpDirectives.size(); i++)
	{
		std::cout << httpDirectives[i] << std::endl;
	}

	std::cout << "@ Server" << std::endl;
	for (int i = 0; i < serverDirectives.size(); i++)
	{
		std::cout << serverDirectives[i] << std::endl;
	}

	std::cout << "@ Location" << std::endl;
	for (int i = 0; i < locationDirectives.size(); i++)
	{
		std::cout << locationDirectives[i] << std::endl;
	}
}

int main()
{
	std::ifstream configFile("/Users/hyeonsok/goinfre/Webserv/webserv/config/default.conf");
	if (!configFile)
	{
		std::cerr << "file not found" << std::endl;
		exit(1);
	}
	std::stringstream configBuffer;
	configBuffer = ft_sed2(configFile, "{", "\n{\n");
	ft_sed3(configBuffer, ";", ";\n");
	ft_sed3(configBuffer, "}", "\n}\n");

	std::vector<std::string> lines;
	while (!configBuffer.eof())
	{
		std::string line;
		std::getline(configBuffer, line);
		boost::trim(line);
		if (line.empty() || line.front() == '#')
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
			struct server &serv = parser.http.serv;
			if (token == "{")
			{
				if (serv.isInBlock)
				{
					std::cout << "line: " << __LINE__ << std::endl;
					print(parser);
					throw std::runtime_error("syntax wrong: context http");
				}
				serv.isInBlock = true;
			}
			else if (token == "}")
			{
				if (!serv.isInBlock)
				{
					std::cout << "line: " << __LINE__ << std::endl;
					print(parser);
					throw std::runtime_error("syntax wrong: context http");
				}
				serv.isInBlock = false;
				parser.context = CONTEXT_HTTP;
			}
			else
			{
				if (!serv.isInBlock)
				{
					std::cout << "line: " << __LINE__ << std::endl;
					print(parser);
					throw std::runtime_error("syntax wrong: context http");
				}
				if (token == "location")
				{
					std::getline(lineBuffer, token, ' ');
					if (token.empty() || token.front() != '/')
					{
						std::cout << "line: " << __LINE__ << std::endl;
						print(parser);
						throw std::runtime_error("syntax wrong: context http");
					}
					serv.loc.url = token;
					parser.context = CONTEXT_LOCATION;
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
					serv.directives.push_back(lines[i].substr(0, lines[i].length() - 1));
				}
			}
			break;
		}
		case CONTEXT_LOCATION:
		{
			struct location &loc = parser.http.serv.loc;
			if (token == "{")
			{
				if (loc.isInBlock)
				{
					std::cout << "line: " << __LINE__ << std::endl;
					print(parser);
					throw std::runtime_error("syntax wrong: context location");
				}
				loc.isInBlock = true;
			}
			else if (token == "}")
			{
				if (!loc.isInBlock)
				{
					std::cout << "line: " << __LINE__ << std::endl;
					print(parser);
					throw std::runtime_error("syntax wrong: context location");
				}
				loc.isInBlock = false;
				parser.context = CONTEXT_SERVER;
			}
			else
			{
				if (!loc.isInBlock)
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
					loc.directives.push_back(lines[i].substr(0, lines[i].length() - 1));
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
