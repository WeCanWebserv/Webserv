#include "request.hpp"

#include <iostream> // TODO: 지우기

Request::Request() : parseStage(STAGE_STARTLINE), requestMessageSize(0), headerbufSize(0) {}

bool Request::detectSectionDelimiter(std::string &line)
{
	if (line.length() == 0 || (line.length() == 1 && line[0] == CR))
		return true;
	return false;
}

bool Request::checkLineFinishedWithoutNewline(std::stringstream &buf)
{
	return buf.eof();
}

void Request::doRequestEpilogue(std::string &line)
{
	this->buf.clear();
	this->buf.str("");
	this->buf << line;
}

void Request::setParseStage(ParseStage stage)
{
	this->parseStage = stage;
}

size_t Request::countParsedOctets(const std::string &line, const size_t &initialBufferLength)
{
	return (line.length() - initialBufferLength + 1); // 개행까지 세어야 하므로 +1을 해줌
}

int Request::fillBuffer(const char *octets, size_t inputLength)
{
	std::string line;
	size_t parsedLength;
	size_t initialLength;

	// TODO: 정해진 content-length 이후에 있는 문자들은 다른 requesetmessage이므로 빼야한다.
	this->requestMessageSize += inputLength;
	parsedLength = 0;
	initialLength = this->buf.str().length();
	this->buf << octets;
	try
	{
		while (this->parseStage != STAGE_BODY && std::getline(this->buf, line))
		{
			if (checkLineFinishedWithoutNewline(this->buf))
			{
				doRequestEpilogue(line); // line이 덜 끝난 경우(= 개행 없이 끝난 경우)
				break;
			}
			parsedLength += countParsedOctets(line, initialLength);
			initialLength = 0;
			switch (this->parseStage)
			{
			case STAGE_STARTLINE:
				if (!detectSectionDelimiter(line)) // CRLF에 대해 trim을 한다.
				{
					RequestParser::startlineParser(startline, line);
					setParseStage(STAGE_HEADER);
				}
				break;
			case STAGE_HEADER:
				if (!detectSectionDelimiter(line))
				{
					RequestParser::fillHeaderBuffer(headerbuf, line, headerbufSize);
					headerbufSize += line.length() + 1;
				}
				else
				{
					RequestParser::headerParser(header, headerbuf, startline.method);
					setParseStage(STAGE_BODY); // TODO: body로 가거나 끝나거나 check
				}
				break;
			default:
				break;
			}
		}
		if (this->buf.eof())
			doRequestEpilogue(line);
	}
	catch (int code)
	{
		std::cerr << "error code: " << code << std::endl;
		return 1;
	}
	if (this->parseStage == STAGE_BODY)
	{
		/**
		 * TODO: content-length인지 chunked transfer-encoding인지 확인
		 * TODO: content-length와 chunked message 관련 오류를 꼼꼼히 확인할 것 -> incomplete message를 판별해야 한다.
		 */
		// RequestParser::bodyParser(body, header.getHeaderMap(), octets, parsedLength, inputLength);
		// if (body->end())
		// 	setParseStage(STAGE_DONE);
	}
	return (this->parseStage == STAGE_DONE);
}