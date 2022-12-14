#include "libft.hpp"

#include <cstring>
#include <sstream>
#include <string>
#include <vector>

std::vector<std::string> ft_split(const std::string &str, char delim)
{
	std::vector<std::string> tokens;
	std::string token;
	std::stringstream strStream;
	strStream.str(str);
	while (!strStream.eof())
	{
		std::getline(strStream, token, delim);
		if (token.empty())
			continue;
		tokens.push_back(token);
	}
	return tokens;
}

void ft_trim(std::string &str, const std::string &ws)
{
	size_t firstIdx = str.find_first_not_of(ws);
	size_t lastIdx = str.find_last_not_of(ws);
	if (firstIdx == std::string::npos && firstIdx == lastIdx)
		str = std::string("");
	else
		str = str.substr(firstIdx, lastIdx + 1);
}

void ft_replace(std::stringstream &strStream, const std::string &s1, const std::string &s2)
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

namespace ft
{
char *strdup(const std::string &str)
{
	char *res;

	res = new char[str.size() + 1];
	res[0] = '\0';
	std::strcpy(res, str.c_str());
	return (res);
}

int toUnderscore(int c)
{
	if (c == '-')
		return ('_');
	return (c);
}
} // namespace ft
