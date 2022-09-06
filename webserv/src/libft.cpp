#include "libft.hpp"

#include <sstream>
#include <string>

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
