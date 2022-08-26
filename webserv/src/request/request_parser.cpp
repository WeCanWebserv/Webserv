#include "request_parser.hpp"

#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#include <iostream> // TODO: 지우기(디버깅용)

std::vector<std::string> RequestParser::methodTokenSet = RequestParser::initMethodTokenSet();
// std::vector<std::string> RequestParser::headerTokenSet = RequestParser::initHeaderTokenSet();

void RequestParser::startlineParser(Startline &startline, const std::string &line)
{
	std::vector<std::string> startlineTokenSet;
	const char *delimiter = " ";

	startlineTokenSet = RequestParser::splitStr(line, delimiter);

	if (startlineTokenSet.size() > 3)
		throw(400);
	RequestParser::startlineMethodParser(startline.method, startlineTokenSet[0]);
	RequestParser::startlineURIParser(startline.uri, startlineTokenSet[1], startline.method);
	RequestParser::startlineHTTPVersionParser(startline.httpVersion, startlineTokenSet[2]);

	std::cout << "[startline]\n";
	std::cout << "startline method: " << startline.method << std::endl;
	std::cout << "startline URI: " << startline.uri << std::endl;
	std::cout << "startline HTTPversion: " << startline.httpVersion << std::endl;
}

void RequestParser::headerParser(Header &header, const std::string &line)
{
	/**
	 * header field에 대한 token부터 만들어두고 parsing하자.
	 */
}

void RequestParser::startlineMethodParser(std::string &method, const std::string &token)
{
	size_t idx;

	/**
	 *  case-sensitive로 확인해야 한다.
	 */
	if ((idx = findToken(token, RequestParser::methodTokenSet) < 0))
		throw(400);
	method = RequestParser::methodTokenSet[idx];
}

void RequestParser::startlineURIParser(std::string &uri,
																			 const std::string &token,
																			 const std::string &method)
{
	/**
	 * 4가지 form에 대해 확인할 것
	 * origin-form
	 * absolute-form
	 * authority-form
	 * asterisk-form
	 * TODO: url encoding도 고려해야 하나?
	 */
	bool result;

	result = false;
	result = result || RequestParser::checkURIOriginForm(uri, token);
	result = result || RequestParser::checkURIAbsoluteForm(uri, token);
	result = result || RequestParser::checkURIAuthorityForm(uri, token, method);
	result = result || RequestParser::checkURIAsteriskForm(uri, token, method);
	if (!result)
		throw(400);
}

bool RequestParser::checkURIOriginForm(std::string &uri, const std::string &token)
{
	std::string::const_reverse_iterator rit1;
	std::string::const_reverse_iterator rit2;

	/** 
	 * absolute-path [ "?" query ]
	 * ex) /where?q=now&name=hihi
	 */
	if (!token.size())
		throw(400);
	if (token[0] != '/')
		return false;
	/**
	 * TODO: uri format에 대해서 더 확인한 후 적용할 것
	 */
	uri = token;
	return true;
}

bool RequestParser::checkURIAbsoluteForm(std::string &uri, const std::string &token)
{
	/**
	 * regex: https?:\/\/(www\.)?[-a-zA-Z0-9@:%._\+~#=]{1,256}\.[a-zA-Z0-9()]{1,6}\b([-a-zA-Z0-9()@:%_\+.~#?&//=]*)
	 * TODO: regex에 맞춰 validation하기
	 */
	if (!strstr(token.c_str(), "http") && !strstr(token.c_str(), "https"))
		return false;
	if (!strstr(token.c_str(), "www"))
		return false;
	uri = token;
	return true;
}

bool RequestParser::checkURIAuthorityForm(std::string &uri,
																					const std::string &token,
																					const std::string &method)
{
	if (!method.compare("CONNECT"))
		return false;
	/**
	 * TODO: uri format에 대해서 더 확인한 후 적용할 것
	 * TODO: userinfo와 @ 구분문자가 있으면 안됨
	 */
	uri = token;
	return true;
}
bool RequestParser::checkURIAsteriskForm(std::string &uri,
																				 const std::string &token,
																				 const std::string &method)
{
	if (!method.compare("OPTIONS") && token.size() == 1 && token[0] == '*')
	{
		uri = token;
		return true;
	}
	return false;
}

void RequestParser::startlineHTTPVersionParser(std::string &httpVersion, const std::string &token)
{
	std::vector<std::string> tokenset;
	std::vector<std::string> versionTokenSet;
	const char *delimiter = "/";
	char *tok;
	size_t idx;

	tok = strtok(const_cast<char *>(token.c_str()), delimiter);
	while (tok)
	{
		tokenset.push_back(tok);
		tok = strtok(NULL, delimiter);
	}
	if (tokenset[0].compare("HTTP"))
		throw(400);
	versionTokenSet.push_back("1.1");
	versionTokenSet.push_back("1.0");
	versionTokenSet.push_back("0.9");
	if ((idx = RequestParser::findToken(tokenset[1], versionTokenSet)))
		throw(400);
	/**
	 * TODO: 버전 호환성을 어떻게 할지 정할 것
	 */
	httpVersion = std::string(tokenset[1]);
}

// int RequestParser::bodyParser(
// 		Body &body, Header::HeaderMap headerMap, char *inputOctets, size_t startPos, size_t inputSize)
// {
// 	size_t contentLength;

// 	if (!headerMap["transfer-encoding"].compare("chunked"))
// 	{
// 		/**
// 		 * 0을 만나면 return 0
// 		 * 0을 만나기 전까진 -1
// 		 */
// 		return RequestParser::chunkedBodyParser(body, inputOctets, startPos, inputSize);
// 	}
// 	else if (headerMap["content-length"].length())
// 	{
// 		/**
// 		 * 42, 42 -> O
// 		 * 52, 42 -> X (throw error)
// 		 */

// 		contentLength = strtol(headerMap["content-length"].c_str(), NULL, 10);
// 		/**
// 		 * ret == 0 -> request message complete
// 		 * ret < 0 -> 아직 더 읽어야함
// 		 * ret > 0 -> 새로운 request message가 겹쳐있는 경우
// 		 */
// 		return RequestParser::nonChunkedBodyParser(body, contentLength, inputOctets, startPos,
// 																							 inputSize);
// 	}
// 	else
// 	{
// 		/**
// 		 * GET이나 HEAD가 아니라면? -> 무조건 content-length 필요
// 		 */
// 	}
// }

// int RequestParser::chunkedBodyParser(Body &body, char *octets, size_t startPos, size_t octetSize)
// {
// 	char *token;
// 	char *bodyOctets = octets + startPos;
// 	size_t bodyOctetSize = octetSize - startPos;
// 	size_t idx;

// 	// token = strtok(bodyOctets, "\r\n");
// 	for (idx = 0; idx < bodyOctetSize; idx++)
// 	{
// 		if (bodyOctets[idx] == '0')
// 		{
// 			break;
// 		}
// 	}
// }

// int RequestParser::nonChunkedBodyParser(
// 		Body &body, size_t contentLength, char *octets, size_t startPos, size_t octetSize)
// {
// 	size_t inputBodySize;
// 	size_t remainBodySize;
// 	ssize_t remainOctetSize;

// 	inputBodySize = octetSize - startPos;
// 	if (body.size() + inputBodySize > contentLength)
// 	{
// 		remainBodySize = contentLength - body.size();
// 		body.insert(body.end(), &octets[startPos], &octets[startPos + remainBodySize - 1]);
// 		remainOctetSize = octetSize - remainBodySize;
// 	}
// 	else
// 	{
// 		body.insert(body.end(), &octets[startPos], &octets[octetSize - 1]);
// 		if (body.size() == contentLength)
// 			remainOctetSize = 0;
// 		else
// 			remainOctetSize = body.size() - contentLength;
// 	}
// 	return remainOctetSize;
// }

/**
 * util static method
 */
size_t RequestParser::findToken(const std::string &token, const std::vector<std::string> &tokenset)
{
	for (size_t i = 0; i < tokenset.size(); i++)
	{
		if (!token.compare(tokenset[i]))
		{
			return i;
		}
	}
	return -1;
}

std::vector<std::string> RequestParser::splitStr(const std::string &str, const char *delimiter)
{
	std::vector<std::string> tokenset;
	char *token;

	token = strtok(const_cast<char *>(str.c_str()), delimiter);
	while (token)
	{
		tokenset.push_back(token);
		token = strtok(NULL, delimiter);
	}
	return tokenset;
}

// std::vector<std::string> RequestParser::initHeaderTokenSet(void)
// {
// 	std::vector<std::string> tokenset;

// 	// tokenset.push_back()
// }

std::vector<std::string> RequestParser::initMethodTokenSet(void)
{
	std::vector<std::string> tokenset;

	tokenset.push_back("GET");
	tokenset.push_back("POST");
	tokenset.push_back("DELETE");
	tokenset.push_back("HEAD");
	tokenset.push_back("PUT");
	tokenset.push_back("PATCH");
	tokenset.push_back("OPTIONS");
	tokenset.push_back("CONNECT");
	tokenset.push_back("TRACE");
	return tokenset;
}
