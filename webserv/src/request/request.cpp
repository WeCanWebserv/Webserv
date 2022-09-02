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

int Request::fillBuffer(const char *octets, size_t octetSize)
{
	std::string line;
	std::string tmp;
	size_t parsedLength;
	size_t initialLength;

	// TODO: 정해진 content-length 이후에 있는 문자들은 다른 requesetmessage이므로 빼야한다.
	this->requestMessageSize += octetSize;
	parsedLength = 0;
	initialLength = this->buf.str().length();
	tmp.append(octets, octetSize);
	this->buf << tmp;
	try
	{
		while (this->parseStage != Request::STAGE_BODY && std::getline(this->buf, line))
		{
			if (checkLineFinishedWithoutNewline(this->buf)) // line이 덜 끝난 경우(= 개행 없이 끝난 경우)
			{
				doRequestEpilogue(line);
				break;
			}
			parsedLength += countParsedOctets(line, initialLength);
			initialLength = 0;
			switch (this->parseStage)
			{
			case Request::STAGE_STARTLINE:
				if (!detectSectionDelimiter(line)) // CRLF에 대해 trim을 한다.
				{
					RequestParser::startlineParser(startline, line);
					setParseStage(Request::STAGE_HEADER);
				}
				break;
			case Request::STAGE_HEADER:
				if (!detectSectionDelimiter(line))
				{
					RequestParser::fillHeaderBuffer(headerbuf, line, headerbufSize);
					headerbufSize += line.length() + 1;
				}
				else
				{
					RequestParser::headerParser(header, headerbuf, startline.method);
					setParseStage(
							Request::
									STAGE_BODY); // body로 가거나 끝나거나 check -> body parser에서 진행됨. 이미 관련 validation을 다 해둔 상태임
				}
				break;
			default:
				break;
			}
		}
		if (this->buf.eof())
			doRequestEpilogue(line);
		if (this->parseStage == Request::STAGE_BODY)
		{
			std::vector<char> bodyOctets(octets + parsedLength, octets + octetSize);
			// TODO: 0보다 클 때 (body를 다 읽고 남을 때: HTTP Pipelining 때문에)에 대한 처리를 해줘야 함
			if (RequestParser::bodyParser(body, bodyOctets, header) >= 0)
			{
				RequestParser::postBodyParser(body, header);
				this->parseStage = Request::STAGE_DONE;
#if DEBUG
				body.print();
#endif
			}
		}
	}
	catch (int code)
	{
		std::cerr << "error code: " << code << std::endl;
		return -1;
	}
	return (this->parseStage == Request::STAGE_DONE);
}

int Request::ready(void)
{
	return this->parseStage == Request::STAGE_DONE;
}