#include "request_parser.hpp"
#include "field_value.hpp"

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
	RequestParser::startlineHTTPVersionParser(startline.httpVersion,
																						RequestParser::trimStr(startlineTokenSet[2], "\r"));
#if DEBUG
	std::cout << "[startline]\n";
	std::cout << "startline method: " << startline.method << std::endl;
	std::cout << "startline URI: " << startline.uri << std::endl;
	std::cout << "startline HTTPversion: " << startline.httpVersion << "\n\n";
#endif
}

void RequestParser::startlineMethodParser(std::string &method, const std::string &token)
{
	ssize_t idx;

	/**
	 *  case-sensitive로 확인해야 한다.
	 */
	if ((idx = RequestParser::findToken(token, RequestParser::methodTokenSet)) < 0)
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
	ssize_t idx;

	/**
	 * TODO: 버전 호환성을 어떻게 할지 정할 것
	 */
	tokenset = RequestParser::splitStr(token, "/");
	if (tokenset[0].compare("HTTP"))
		throw(400);
	versionTokenSet = RequestParser::splitStr(versionList, ", ");
	if ((idx = RequestParser::findToken(tokenset[1], versionTokenSet)) < 0)
		throw(400);
	httpVersion = tokenset[1];
}

void RequestParser::fillHeaderBuffer(std::map<std::string, std::string> &headerbuf,
																		 const std::string &line,
																		 const size_t headerbufSize)
{
	std::vector<std::string> headerTokenSet;
	std::vector<std::string> tmp;
	std::string tmpstring;
	/**
	 * 1. header 분리 (CRLF 기준으로 header각각을 분리)
	 * 2. header field와 value를 분리
	 * 3. field에 SP가 있으면 400 error (공격 방지를 위함)
	 * 4. value는 ":" 바로 뒤에 오는 SP문자를 제외한 SP문자 및 CRLF 바로 앞에 오는 SP를 제외한 SP문자들을 trim 한다.
	 * 5. header section이 모두 끝날때까지 위의 1~4번을 반복한다.
	 * 
	 * header section이 끝나면 headerParser메소드에서 로직 시작
	 */
	if (headerbufSize + line.length() + 1 > RequestParser::maxHeaderSize)
		throw(431); // rfc6585.5
	headerTokenSet = RequestParser::splitStr(line, "\r\n");
	for (size_t i = 0; i < headerTokenSet.size(); i++)
	{
		tmp = RequestParser::splitStr(headerTokenSet[i], ":");
		if (tmp.size() < 2 || RequestParser::checkHeaderFieldnameHasSpace(tmp[0]))
			throw(400);
		if (tmp[0].length() > RequestParser::maxHeaderFieldSize)
			throw(431);
		tmp[0] = RequestParser::tolowerStr(tmp[0].c_str());
		tmp[1] = line;
		tmp[1] = RequestParser::trimStr(tmp[1].erase(0, tmp[0].size() + 1), " ");
		if (RequestParser::checkHeaderFieldContain(headerbuf, tmp[0]))
		{
			tmpstring = RequestParser::trimStr(tmp[1], " ") + std::string(", ");
			headerbuf[tmp[0]] = tmpstring + headerbuf[tmp[0]];
		}
		else
		{
			headerbuf[tmp[0]] = RequestParser::trimStr(tmp[1], " ");
		}
	}

#if DEBUG > 2
	std::map<std::string, std::string>::iterator it = headerbuf.begin();
	std::map<std::string, std::string>::iterator end = headerbuf.end();
	size_t idx = 0;
	for (; it != end; it++, idx++)
	{
		std::cout << "[" << idx << "] " << it->first << ": " << it->second << std::endl;
	}
#endif
}

void RequestParser::headerParser(Header &header,
																 std::map<std::string, std::string> &headerbuf,
																 const std::string &method)
{
	/**
 	 * header field에 없는 token은 무시하도록 하자(error가 아님)
	 * header-field는 오로지 US_ASCII로만 encoding되어야 한다.
	 * header-value에 대한 format은 복잡하니 정리해놓은 것을 참고할 것.
	 * 
	 * 6. header section이 모두 끝나면 predefined field name에 맞지 않는 것은 누락시키고 Header 구조에 맞춰 넣는다.
	 * 7. 그리고 field value에 대해 validation을 진행한다.
	 * 8. invalid format이라면 해당 이유에 맞춰 status code를 throw한다.
	 * 9. valid format이라면 FieldValue 구조에 맞춰 parsing 및 할당한다.
	 */
#if DEBUG
	std::cout << "header section is done\n";
#endif
	std::vector<FieldValue> fieldvalueVec;

	if (!RequestParser::checkHeaderFieldContain(headerbuf, "Host"))
		throw(400);
	if (RequestParser::checkHeaderFieldContain(headerbuf, "Transfer-Encoding") &&
			::strstr(headerbuf[RequestParser::tolowerStr("Transfer-Encoding")].c_str(), "chunked"))
	{
		if (RequestParser::checkHeaderFieldContain(headerbuf, "Content-Length"))
			headerbuf[RequestParser::tolowerStr("Transfer-Encoding")] = "";
	}
	else
	{
		if (!RequestParser::checkHeaderFieldContain(headerbuf, "Content-Length"))
		{
#if DEBUG
			std::cout << "no chunked and content-length\n";
#endif
			throw(411); // length required error
		}
	}
	for (std::map<std::string, std::string>::const_iterator it = headerbuf.begin();
			 it != headerbuf.end(); it++)
	{
		RequestParser::headerValueParser(fieldvalueVec, it->second);
		header.headerMap[RequestParser::tolowerStr(it->first.c_str())] = fieldvalueVec;
		fieldvalueVec.clear();
	}
#if DEBUG
	header.print();
#endif
}

void RequestParser::headerValueParser(std::vector<FieldValue> &fieldvalueVec,
																			const std::string &headerValue)
{
	std::vector<std::string> valueTokenSet;
	std::vector<std::string> descriptionTokenSet;

	valueTokenSet = splitStr(headerValue, ",");
	for (size_t i = 0; i < valueTokenSet.size(); i++)
	{
		descriptionTokenSet =
				RequestParser::splitStr(RequestParser::trimStr(valueTokenSet[i], " "), ";");
		if (descriptionTokenSet.size() < 1)
			continue;
		FieldValue fieldvalue;
		fieldvalue.value = descriptionTokenSet[0];
		descriptionTokenSet.erase(descriptionTokenSet.begin());
		RequestParser::headerValueDescriptionParser(fieldvalue.descriptions, descriptionTokenSet);
		fieldvalueVec.push_back(fieldvalue);
	}
}

void RequestParser::headerValueDescriptionParser(std::map<std::string, std::string> &descriptions,
																								 std::vector<std::string> &descriptionTokenSet)
{
	std::vector<std::string> pairTokenSet;
	const std::string blank = "";

	for (size_t i = 0; i < descriptionTokenSet.size(); i++)
	{
		pairTokenSet =
				RequestParser::splitStr(RequestParser::trimStr(descriptionTokenSet[i], " "), "=");
		if (pairTokenSet.size() < 1)
			continue;
		if (pairTokenSet.size() < 2)
		{
			descriptions.insert(std::make_pair(pairTokenSet[0], blank));
		}
		else if (pairTokenSet.size() == 2)
			descriptions.insert(std::make_pair(pairTokenSet[0], pairTokenSet[1]));
		else
		{
			std::string tmp;
			for (size_t j = 1; j < pairTokenSet.size(); j++)
				tmp = tmp + pairTokenSet[i];
			descriptions.insert(std::make_pair(pairTokenSet[0], tmp));
		}
	}
}

bool RequestParser::checkHeaderFieldnameHasSpace(const std::string &fieldname)
{
	return (fieldname.find(' ') != std::string::npos);
}

bool RequestParser::checkHeaderFieldContain(std::map<std::string, std::string> &headerbuf,
																						const std::string fieldname)
{
	return (headerbuf[RequestParser::tolowerStr(fieldname.c_str())].size() > 0);
}

/**
 * For Body Parser
 */
ssize_t RequestParser::bodyParser(Body &body, std::vector<char> &bodyOctets, Header &header)
{
	if (header.headerMap[RequestParser::tolowerStr("Transfer-Encoding")].size())
	{
		std::vector<FieldValue> values =
				header.headerMap[RequestParser::tolowerStr("Transfer-Encoding")];
		for (size_t i = 0; i < values.size(); i++)
		{
			if (values[i].value == "chunked")
				return RequestParser::chunkedBodyParser(body, bodyOctets);
		}
	}
	else if (header.headerMap[RequestParser::tolowerStr("Content-Length")].size())
	{
		return RequestParser::contentLengthBodyParser(body, bodyOctets, header);
	}
	/**
	 * GET이나 HEAD일 경우에 해당한다.
	 * 나머지 경우는 HeaderParser가 완성된 후로 validation을 완료 하였다.
	 */
	body.setParseFlag(Body::FINISHED);
	return (0);
}

ssize_t RequestParser::chunkedBodyParser(Body &body, std::vector<char> &bodyOctets)
{
	ssize_t lineLength;
	std::vector<char> lineBuffer;
	std::vector<std::vector<char> > lineTokenSet;
	// TODO: 입력으로 들어오는 bodyOctets를 변경하지 말고 새로 지역변수를 복사생성자로 생성하여 이를 이용하자

	body.chunkedParsingPrologue(lineBuffer, lineLength);
	while (bodyOctets.size())
	{
		if (body.getParseFlag() == Body::CHUNKED_LENGTH)
		{
			// parse length and save length
			if ((lineLength = RequestParser::parseChunkedLengthLine(body, bodyOctets, lineBuffer)) < 0)
				break;
			if (!lineLength)
			{
				body.setParseFlag(Body::FINISHED);
				break;
			}
			body.setParseFlag(Body::CHUNKED_CONTENT);
		}
		else if (body.getParseFlag() == Body::CHUNKED_CONTENT)
		{
			// parse body octets with saved length
			if (!RequestParser::parseChunkedContentLine(body, bodyOctets, lineBuffer, lineLength))
			{
				lineLength = 0;
				body.setParseFlag(Body::CHUNKED_LENGTH);
			}
			else
				break;
		}
	}
	if (body.getParseFlag() == Body::FINISHED)
	{
#if DEBUG
		std::cout << "body parse finished\n";
#endif
		return (0);
	}
	body.chunkedParsingEpilogue(lineBuffer, lineLength);
	return (
			-1); // TODO: 모자르다는 의미, 근데 들어온 입력이 남는다는 의미도 표현해야 한다. (-1)로 할 것임
}

ssize_t RequestParser::parseChunkedLengthLine(Body &body,
																							std::vector<char> &bodyOctets,
																							std::vector<char> &lineBuffer)
{
	size_t i;
	ssize_t length = -1;

	for (i = 0; i < bodyOctets.size(); i++)
	{
		if (bodyOctets[i] == '\r')
		{
			if (i + 1 < bodyOctets.size() && bodyOctets[i + 1] == '\n')
			{
				body.setParseFlag(Body::CHUNKED_CONTENT);
				bodyOctets.erase(bodyOctets.begin(), bodyOctets.begin() + (i + 2));
				const char *cstr = RequestParser::vecToCstr(lineBuffer, lineBuffer.size());
				length = ::strtol(cstr, NULL, 16);
				lineBuffer.clear();
				delete[] cstr;
			}
			else
				lineBuffer.push_back(bodyOctets[i]);
			break;
		}
		if (bodyOctets[i] == '\n')
		{
			if (lineBuffer[lineBuffer.size() - 1] != '\r')
				throw(400);
			else
				lineBuffer.pop_back();
			body.setParseFlag(Body::CHUNKED_CONTENT);
			bodyOctets.erase(bodyOctets.begin(), bodyOctets.begin() + (i + 1));
			const char *cstr = RequestParser::vecToCstr(lineBuffer, lineBuffer.size());
			length = ::strtol(cstr, NULL, 16);
			lineBuffer.clear();
			delete[] cstr;
			break;
		}
		if (!::isxdigit(bodyOctets[i]))
		{
			throw(400); // TODO: throw 외에 다른 예외 처리 방법 생각해보기(review 받기)
		}
		lineBuffer.push_back(bodyOctets[i]);
	}
	return length;
}

ssize_t RequestParser::parseChunkedContentLine(Body &body,
																							 std::vector<char> &bodyOctets,
																							 std::vector<char> &lineBuffer,
																							 ssize_t lineLength)
{
	ssize_t i;
	ssize_t savedlength = lineBuffer.size();

	for (i = 0; i < bodyOctets.size(); i++)
	{
		if (i + savedlength == lineLength)
		{
			body.payload.insert(body.payload.end(), lineBuffer.begin(), lineBuffer.end());
			lineBuffer.clear();
			bodyOctets.erase(bodyOctets.begin(), bodyOctets.begin() + i + 2); // \r\n도 지우기 위해
			return (0);
		}
		lineBuffer.push_back(bodyOctets[i]);
	}
	bodyOctets.clear();
	return (i - lineLength);
}

ssize_t
RequestParser::contentLengthBodyParser(Body &body, std::vector<char> &bodyOctets, Header &header)
{
	/**
	 * 1. header의 content-length와 body의 payload의 size를 비교
	 * 2. 남은 size와 들어온 bodyOctets의 size를 비교
	 * 3. 남은 size가 bodyOctets size보다 더 크거나 같다면 모두 body.payload에 insert
	 * 4. 남은 size가 bodyOctets size보다 더 작다면 그 작은 만큼 body.payload에 insert
	 */
	size_t targetSize;
	size_t remainPayloadSize;
	size_t inputPayloadSize;

	targetSize = ::strtol(
			header.headerMap[RequestParser::tolowerStr("Content-Length")][0].value.c_str(), NULL, 10);
	remainPayloadSize = targetSize - body.payload.size();
	inputPayloadSize = bodyOctets.size();
	if (remainPayloadSize >= inputPayloadSize)
	{
		body.payload.insert(body.payload.end(), bodyOctets.begin(), bodyOctets.end());
	}
	else
	{
		body.payload.insert(body.payload.end(), bodyOctets.begin(),
												bodyOctets.begin() + (inputPayloadSize - remainPayloadSize));
	}
	return (inputPayloadSize - remainPayloadSize);
}

/**
 * util static method
 */
ssize_t RequestParser::findToken(const std::string &token, const std::vector<std::string> &tokenset)
{
	for (size_t i = 0; i < tokenset.size(); i++)
	{
		if (token == tokenset[i])
			return i;
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

// TODO: 입력의 변경을 없애자(입력을 변경하는 대신 새로운 변수를 만들어 반환하자)
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

std::string RequestParser::tolowerStr(const char *str)
{
	std::string copystr;

	copystr = str;
	std::transform(copystr.begin(), copystr.end(), copystr.begin(), ::tolower);
	return copystr;
}

/**
 * @description: do memory allocation
 */
char *RequestParser::vecToCstr(const std::vector<char> &vec, size_t size)
{
	char *octets;

	octets = new char[size + 1];
	memcpy(octets, &vec[0], size);
	octets[size] = '\0';
	return octets;
}

std::vector<char> RequestParser::cstrToVec(const char *cstr, size_t size)
{
	return (std::vector<char>(cstr, cstr + size));
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
