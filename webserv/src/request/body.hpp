#ifndef __FT_BODY_H__
#define __FT_BODY_H__

#include "field_value.hpp"
#include "header.hpp"
#include <map>
#include <string>
#include <vector>

struct Body
{
	std::vector<char> payload;
	std::vector<std::pair<Header, std::vector<char> > > multipartFormData;
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

#if DEBUG
public:
	void print(void)
	{
		std::cout << "\n################### [ DEBUG: Body print ] ##################\n\n";
		for (size_t i = 0; i < this->payload.size(); i++)
		{
			std::cout << this->payload[i];
		}
		std::cout <<"\n\n##########################################################\n";
	}
#endif

private:
	ParseFlag parseFlag;
	std::vector<char> lineBuffer;
	ssize_t lineLength;
};

#endif
