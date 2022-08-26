#ifndef __FT_REQUEST_H__
#define __FT_REQUEST_H__

#include "body.hpp"
#include "header.hpp"
#include "request_parser.hpp"
#include "startline.hpp"

#include <map>
#include <sstream>

#define CR 0x0d
#define LF 0x0a

class Request
{
private:
	enum ParseStage
	{
		STAGE_ERROR = -1,
		STAGE_STARTLINE,
		STAGE_HEADER,
		STAGE_BODY,
		STAGE_DONE
	};

	Startline startline;
	Header header;
	Body body;

	ParseStage parseStage;
	std::stringstream buf; // for startline and header
	size_t requestMessageSize;

public:
	Request();
	// ~Request(); // remove buffer
	int fillBuffer(char *octets, size_t len);
	int end(void); // return 1 when message completed

private:
	void setParseStage(ParseStage stage);
	bool detectSectionDelimiter(std::string &line);
	bool checkLineFinishedWithoutNewline(std::stringstream &buf);
	void doRequestEpilogue(std::string &line);
};

#endif
