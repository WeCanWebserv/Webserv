#ifndef __FT_REQUEST_PARSER_H__
#define __FT_REQUEST_PARSER_H__

#include "body.hpp"
#include "header.hpp"
#include "startline.hpp"
#include <map>
#include <string>
#include <vector>

struct RequestParser
{
	static void startlineParser(Startline &startline, const std::string &str);
	static void headerParser(Header &header,
													 std::map<std::string, std::string> &headerbuf,
													 const std::string &method);
	static void fillHeaderBuffer(std::map<std::string, std::string> &headerbuf,
															 const std::string &str,
															 const size_t headerbufSize);
	static ssize_t bodyParser(Body &body, std::vector<char> &bodyOctets, Header &header);
	static void postBodyParser(Body &body, Header &header);

private:
	static std::vector<std::string> methodTokenSet;
	static const size_t maxHeaderSize = 8 * 1024;
	static const size_t maxHeaderFieldSize = 30 * 1024;

	static std::vector<std::string> initMethodTokenSet(void);
	/**
	 * startline parser util
	 */
	static void startlineMethodParser(std::string &method, const std::string &token);
	static void
	startlineURIParser(std::string &uri, const std::string &token, const std::string &method);
	static void startlineHTTPVersionParser(std::string &httpVersion, const std::string &token);
	// static bool validateHeaderValue(Header &header);

	/**
	 * header parser util
	 */
	static bool checkHeaderFieldnameHasSpace(const std::string &fieldname);
	static bool
	checkHeaderFieldContain(std::map<std::string, std::string> &headerbuf, const std::string str);
	static void
	headerValueParser(std::vector<FieldValue> &fieldvalue, const std::string &headerValue);
	static void headerValueDescriptionParser(std::map<std::string, std::string> &descriptions,
																					 std::vector<std::string> &descriptionsTokenSet);
	static bool validateHeaderField(Header &header);
	/**
	 * body parser util
	 */
	static ssize_t chunkedBodyParser(Body &body, std::vector<char> &bodyOctets);
	static ssize_t contentLengthBodyParser(Body &body, std::vector<char> &bodyOctets, Header &header);
	static ssize_t
	parseChunkedLengthLine(Body &body, std::vector<char> &bodyOctets, std::vector<char> &lineBuffer);
	static ssize_t parseChunkedContentLine(Body &body,
																				 std::vector<char> &bodyOctets,
																				 std::vector<char> &lineBuffer,
																				 ssize_t lineLength);
	static void parseMultipartBody(Body &body, const std::string &boundary);
	static void parseMultipartEachBody(Body &body, const std::string &eachBody);
	static void parseMultipartEachHeader(Body &body,
																			 const std::string &headerSection,
																			 const std::string &bodySection);

	/**
	 * uri parser util
	 */
	static bool checkURIOriginForm(std::string &uri, const std::string &token);
	static bool checkURIAbsoluteForm(std::string &uri, const std::string &token);
	static bool
	checkURIAuthorityForm(std::string &uri, const std::string &token, const std::string &method);
	static bool
	checkURIAsteriskForm(std::string &uri, const std::string &token, const std::string &method);

	static ssize_t findToken(const std::string &token, const std::vector<std::string> &tokenset);
	static std::vector<std::string> splitStr(const std::string &token, const char *delimiter);
	static std::vector<std::string>
	splitStrStrict(const std::string &token, const char *delimiter, size_t delimiterSize);
	static std::string &trimStr(std::string &target, const std::string &charset);
	static std::string &trimStrStrict(std::string &target, const std::string &charset);
	static std::string tolowerStr(const char *str);
};

#endif