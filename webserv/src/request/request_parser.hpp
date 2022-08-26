#ifndef __FT_REQUEST_PARSER_H__
#define __FT_REQUEST_PARSER_H__

#include "body.hpp"
#include "header.hpp"
#include "startline.hpp"
#include <string>
#include <vector>

struct RequestParser
{
	static void startlineParser(Startline &startline, const std::string &str);
	static void headerParser(Header &header, const std::string &str);
	// static int bodyParser(Body &body, Header::HeaderMap headerMap, char *octets, size_t startPos, size_t octetSize);

private:
	static std::vector<std::string> methodTokenSet;
	static std::vector<std::string> headerTokenSet;

	static std::vector<std::string> initMethodTokenSet(void);
	// static std::vector<std::string> initHeaderTokenSet(void);

	static void startlineMethodParser(std::string &method, const std::string &token);
	static void
	startlineURIParser(std::string &uri, const std::string &token, const std::string &method);
	static void startlineHTTPVersionParser(std::string &httpVersion, const std::string &token);

	// static int chunkedBodyParser(Body &body, char *octets, size_t startPos, size_t octetSize);
	// static int nonChunkedBodyParser(Body &body, size_t contentLength, char *octets, size_t startPos, size_t octetSize);

	/**
	 * uri parser util
	 */
	static bool checkURIOriginForm(std::string &uri, const std::string &token);
	static bool checkURIAbsoluteForm(std::string &uri, const std::string &token);
	static bool
	checkURIAuthorityForm(std::string &uri, const std::string &token, const std::string &method);
	static bool
	checkURIAsteriskForm(std::string &uri, const std::string &token, const std::string &method);

	static size_t findToken(const std::string &token, const std::vector<std::string> &tokenset);
	static std::vector<std::string> splitStr(const std::string &token, const char *delimiter);
};




#endif