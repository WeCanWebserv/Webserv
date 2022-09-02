#ifndef __FT_REQUEST_H__
#define __FT_REQUEST_H__

#include "body.hpp"
#include "field_value.hpp"
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

	typedef std::vector<FieldValue> FieldValueList;

	Startline startline;
	Header header;
	Body body;

	ParseStage parseStage;
	std::stringstream buf; // for startline and header
	size_t requestMessageSize;
	std::map<std::string, std::string> headerbuf;
	size_t headerbufSize;

public:
	Request();
	// ~Request();
	int fillBuffer(const char *octets, size_t len);
	int ready(void); // return 1 when message completed
	const Startline& getStartline(void) const;
	const Header& getHeader(void) const;
	const Body& getBody(void) const;

private:
	void setParseStage(ParseStage stage);
	bool detectSectionDelimiter(std::string &line);
	bool checkLineFinishedWithoutNewline(std::stringstream &buf);
	void doRequestEpilogue(std::string &line);
	size_t countParsedOctets(const std::string &line, const size_t &initialBufferLength);
};

#endif
