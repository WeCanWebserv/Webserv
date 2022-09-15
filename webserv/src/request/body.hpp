#ifndef __FT_BODY_H__
#define __FT_BODY_H__

#include "field_value.hpp"
#include "header.hpp"
#include <sstream>
#include <map>
#include <string>
#include <vector>

#include "../Logger.hpp"

class RequestParser;

class Body
{
public:
	std::vector<char> payload;
	std::vector<std::pair<Header, std::vector<char> > > multipartFormData;

	static std::string vecToStr(std::vector<char> vec)
	{
		return (std::string(vec.begin(), vec.end()));
	}
	static std::vector<char> vecToStr(const std::string &str)
	{
		return (std::vector<char>(str.begin(), str.end()));
	}

	Body() : parseFlag(CHUNKED_LENGTH), lineLength(0) {}

private:
	enum ParseFlag
	{
		CHUNKED_LENGTH,
		CHUNKED_CONTENT,
		FINISHED
	};

	friend class RequestParser;
	ParseFlag parseFlag;
	std::vector<char> lineBuffer;
	ssize_t lineLength;

	void chunkedParsingPrologue(std::vector<char> &previousBuffer, ssize_t &previousLength)
	{
		previousBuffer = this->lineBuffer;
		previousLength = this->lineLength;
	}

	void chunkedParsingEpilogue(const std::vector<char> &remain, ssize_t targetLength)
	{
		this->lineBuffer = remain;
		this->lineLength = targetLength;
	}

	ParseFlag getParseFlag(void)
	{
		return parseFlag;
	}

	void setParseFlag(ParseFlag flag)
	{
		parseFlag = flag;
	}

public:
	void print(void)
	{
		std::stringstream ss;

		ss << "\n################### [ DEBUG: Body print ] ##################\n\n";
		ss << std::string(this->payload.begin(), this->payload.end());

		// #if DEBUG > 2
		if (this->multipartFormData.size())
		{
			ss << "\n\n############################ [ Body: Multipart Form Data ] "
						"##############################\n";
			for (size_t i = 0; i < this->multipartFormData.size(); i++)
			{
				ss << "\n########################## < Header " << i << " > ##########################"
					 << std::endl;
				Logger::log(Logger::LOGLEVEL_INFO) << ss.str() << "\n";

				this->multipartFormData[i].first.print();

				ss.str("");
				ss << "\n########################## < Body > ##########################" << std::endl;
				for (size_t j = 0; j < this->multipartFormData[i].second.size(); j++)
					ss << this->multipartFormData[i].second[j];
				ss << "\n\n";
			}
			ss << "######################################################################################"
						"#####\n";
		}
		ss << "\n##########################################################\n";
		Logger::log(Logger::LOGLEVEL_INFO) << ss.str() << "\n";
		// #endif
	}
};

#endif
