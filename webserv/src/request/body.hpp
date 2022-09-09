#ifndef __FT_BODY_H__
#define __FT_BODY_H__

#include "field_value.hpp"
#include "header.hpp"
#include <map>
#include <string>
#include <vector>

#include "../Logger.hpp"

struct Body
{
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

	enum ParseFlag
	{
		CHUNKED_LENGTH,
		CHUNKED_CONTENT,
		FINISHED
	};

	Body() : parseFlag(CHUNKED_LENGTH), lineLength(0) {}

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
		std::string str;

		str.append("\n################### [ DEBUG: Body print ] ##################\n\n");
		str.append(this->payload.begin(), this->payload.end());
		str.append("\n\n##########################################################\n");
		Logger::log(Logger::LOGLEVEL_INFO) << str << "\n";
#if DEBUG
		std::cout << str << std::endl;
#endif

#if DEBUG > 2
		if (this->multipartFormData.size())
		{
			str.clear();
			Logger::log(Logger::LOGLEVEL_INFO) << "#########[ Body: Multipart Form Data ]#########\n";
			for (size_t i = 0; i < this->multipartFormData.size(); i++)
			{
				Logger::log(Logger::LOGLEVEL_INFO) << "\n[[[[[[[ Header " << i << " ]]]]]]]" << std::endl;
				this->multipartFormData[i].first.print();
				Logger::log(Logger::LOGLEVEL_INFO) << "\n[[[[[[[ Body ]]]]]]]" << std::endl;
				for (size_t j = 0; j < this->multipartFormData[i].second.size(); j++)
					Logger::log(Logger::LOGLEVEL_INFO) << this->multipartFormData[i].second[j];
				Logger::log(Logger::LOGLEVEL_INFO) << "\n\n";
			}
		}
#endif
	}

private:
	ParseFlag parseFlag;
	std::vector<char> lineBuffer;
	ssize_t lineLength;
};

#endif
