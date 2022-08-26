#include "request_parser.hpp"

#include <cstdlib>
#include <cstring>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#if DEBUG
#include <iostream> // TODO: 지우기(디버깅용)
#endif

std::vector<std::string> RequestParser::methodTokenSet = RequestParser::initMethodTokenSet();

void RequestParser::startlineParser(Startline &startline, const std::string &line)
{
	std::vector<std::string> startlineTokenSet;

	startlineTokenSet = RequestParser::splitStr(line, " ");
	if (startlineTokenSet.size() > 3)
		throw(400);

	RequestParser::startlineMethodParser(startline.method, startlineTokenSet[0]);
	RequestParser::startlineURIParser(startline.uri, startlineTokenSet[1], startline.method);
	RequestParser::startlineHTTPVersionParser(startline.httpVersion, startlineTokenSet[2]);
#if DEBUG
	std::cout << "[startline]\n";
	std::cout << "startline method: " << startline.method << std::endl;
	std::cout << "startline URI: " << startline.uri << std::endl;
	std::cout << "startline HTTPversion: " << startline.httpVersion << "\n\n";
#endif
}

void RequestParser::startlineMethodParser(std::string &method, const std::string &token)
{
	size_t idx;

	/**
	 *  case-sensitive로 확인해야 한다.
	 */
	if ((idx = RequestParser::findToken(token, RequestParser::methodTokenSet) < 0))
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
	const char *versionList = "3, 2, 1.1, 1.0, 0.9";
	size_t idx;

	/**
	 * TODO: 버전 호환성을 어떻게 할지 정할 것
	 */
	tokenset = RequestParser::splitStr(token, "/");
	if (tokenset[0].compare("HTTP"))
		throw(400);
	versionTokenSet = RequestParser::splitStr(versionList, ", ");
#if DEBUG
	for (size_t i = 0; i < versionTokenSet.size(); i++)
	{
		std::cout << "version[" << i << "]: " << versionTokenSet[i] << std::endl;
	}
#endif
	if ((idx = RequestParser::findToken(tokenset[1], versionTokenSet)) < 0)
		throw(400);
	httpVersion = std::string(tokenset[1]);
}

void RequestParser::fillHeaderBuffer(std::map<std::string, std::string> &headerbuf,
																		 const std::string &line)
{
	std::vector<std::string> headerTokenSet;
	std::vector<std::string> tmp;
	std::string tmpstring;
	/**
	 * header field에 없는 token은 무시하도록 하자(error가 아님)
	 * header-field는 오로지 US_ASCII로만 encoding되어야 한다.
	 * header-value에 대한 format은 복잡하니 정리해놓은 것을 참고할 것.
	 * 
	 * 1. header 분리 (CRLF 기준으로 header각각을 분리)
	 * 2. header field와 value를 분리
	 * 3. field에 SP가 있으면 400 error (공격 방지를 위함)
	 * 4. value는 ":" 바로 뒤에 오는 SP문자를 제외한 SP문자 및 CRLF 바로 앞에 오는 SP를 제외한 SP문자들을 trim 한다.
	 * 5. header section이 모두 끝날때까지 위의 1~4번을 반복한다.
	 * 
	 * header section이 끝나면 headerParser메소드에서 로직 시작
	 */

	headerTokenSet = RequestParser::splitStr(line, "\r\n");
	for (size_t i = 0; i < headerTokenSet.size(); i++)
	{
		tmp = RequestParser::splitStr(headerTokenSet[i], ":");
		if (RequestParser::checkHeaderFieldnameHasSpace(tmp[0]))
			throw(400);
		if (headerbuf.find(tmp[0]) == headerbuf.end())
		{
			headerbuf[tmp[0]] = RequestParser::trimStr(tmp[1], " ");
		}
		else
		{
			tmpstring = RequestParser::trimStr(tmp[1], " ") + std::string(", ");
			headerbuf[tmp[0]] = tmpstring + headerbuf[tmp[0]];
		}
	}

#if DEBUG
	std::map<std::string, std::string>::iterator it = headerbuf.begin();
	std::map<std::string, std::string>::iterator end = headerbuf.end();
	size_t idx = 0;
	for (; it != end; it++, idx++)
	{
		std::cout << "[" << idx << "] " << it->first << ": " << it->second << std::endl;
	}
#endif
}

void RequestParser::headerParser(Header &header, std::map<std::string, std::string> &headerbuf)
{
/**
	 * 6. header section이 모두 끝나면 predefined field name에 맞지 않는 것은 누락시키고 Header 구조에 맞춰 넣는다.
	 * 7. 그리고 field value에 대해 validation을 진행한다.
	 * 8. invalid format이라면 해당 이유에 맞춰 status code를 throw한다.
	 * 9. valid format이라면 FieldValue 구조에 맞춰 parsing 및 할당한다.
	 */
#if DEBUG
	std::cout << "header section is done\n";
#endif
}

bool RequestParser::checkHeaderFieldnameHasSpace(const std::string &fieldname)
{
	return (fieldname.find(' ') != std::string::npos);
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

std::string &RequestParser::trimStr(std::string &target, const std::string &charset)
{
	size_t idx;
	std::string::iterator it;

	idx = 0;
	while (idx < target.size())
	{
		it = target.begin();
		if (charset.find(target[idx]) == std::string::npos)
			break;
		target.erase(it + idx);
	}

	idx = target.size() - 1;
	while (idx >= 0)
	{
		it = target.begin();
		if (charset.find(target[idx]) == std::string::npos)
			break;
		target.erase(it + idx);
		--idx;
	}
	return target;
}

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

/**
 * @deprecated
 */
// std::set<std::string> RequestParser::initHeaderTokenSet(void)
// {
// 	std::vector<std::string> tokenvec;

// 	/**
// 	 * Representation
// 	 */
// 	tokenvec.push_back("Content-Type");
// 	tokenvec.push_back("Content-Encoding");
// 	tokenvec.push_back("Content-Language");
// 	tokenvec.push_back("Content-Length");

// 	/**
// 	 * Negotiation
// 	 */
// 	tokenvec.push_back("Accept");
// 	tokenvec.push_back("Accept-Charset");
// 	tokenvec.push_back("Accept-Encoding");
// 	tokenvec.push_back("Accept-Language");

// 	/**
// 	 * Transfer
// 	 */
// 	tokenvec.push_back("Transfer-Encoding");
// 	tokenvec.push_back("Content-Range");
// 	tokenvec.push_back("Range");
// 	tokenvec.push_back("If-Range");

// 	/**
// 	 * General information
// 	 */
// 	tokenvec.push_back("From");
// 	tokenvec.push_back("Referer");
// 	tokenvec.push_back("User-Agent");
// 	tokenvec.push_back("Date");

// 	/**
// 	 * Special information
// 	 */
// 	tokenvec.push_back("Host");
// 	tokenvec.push_back("Authorization");
// 	tokenvec.push_back("Origin");
// 	tokenvec.push_back("Cookie");
// 	tokenvec.push_back("Connection");

// 	/**
// 	 * Cache
// 	 */
// 	tokenvec.push_back("Cache-Control");
// 	tokenvec.push_back("If-Modified-Since");
// 	tokenvec.push_back("If-Unmodified-Since");
// 	tokenvec.push_back("If-None-Match");
// 	tokenvec.push_back("If-Match");

// 	std::vector<std::string>::iterator end = tokenvec.end();
// 	for (size_t i = 0; i < tokenvec.size(); i++)
// 		std::transform(tokenvec[i].begin(), tokenvec[i].end(), tokenvec[i].begin(), ::tolower);
// #if DEBUG
// 	std::cout << "[ All Header Field ]\n\n";
// 	for (size_t i = 0; i < tokenvec.size(); i++)
// 		std::cout << "Header[" << i << "] : " << tokenvec[i] << std::endl;
// #endif
// 	return std::set<std::string>(tokenvec.begin(), tokenvec.end());
// }