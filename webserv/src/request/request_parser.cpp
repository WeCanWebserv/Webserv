#include "request_parser.hpp"
#include "../Logger.hpp"
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

	startlineTokenSet = splitStr(line, " ");
	if (startlineTokenSet.size() != 3)
	{
		Logger::debug(LOG_LINE) << "Startline token count is not 3";
		throw(400);
	}

	RequestParser::startlineMethodParser(startline.method, startlineTokenSet[0]);
	RequestParser::startlineURIParser(startline.uri, startlineTokenSet[1], startline.method);
	RequestParser::startlineHTTPVersionParser(startline.httpVersion,
																						trimStr(startlineTokenSet[2], "\r"));
	Logger::log(Logger::LOGLEVEL_INFO)
			<< "\n[ Parsed Startline Info ]\nMethod: " << startline.method << "\nURI: " << startline.uri
			<< "\nHTTP Version: " << startline.httpVersion << "\n\n";
}

void RequestParser::startlineMethodParser(std::string &method, const std::string &token)
{
	ssize_t idx;

	/**
	 *  case-sensitive로 확인해야 한다.
	 */
	if ((idx = findToken(token, RequestParser::methodTokenSet)) < 0)
	{
		Logger::debug(LOG_LINE) << "No match method in Method Tokens";
		throw(400);
	}
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
	{
		Logger::debug(LOG_LINE) << "No matched uri form";
		throw(400);
	}
}

bool RequestParser::checkURIOriginForm(std::string &uri, const std::string &token)
{
	std::string::const_reverse_iterator rit1;
	std::string::const_reverse_iterator rit2;

	/**
	 * absolute-path [ "?" query ]
	 * ex) /where?q=now&name=hihi
	 */
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
	tokenset = splitStr(token, "/");
	if (tokenset[0].compare("HTTP"))
	{
		Logger::debug(LOG_LINE) << "Protocol is not HTTP";
		throw(400);
	}
	versionTokenSet = splitStr(versionList, ", ");
	if ((idx = findToken(tokenset[1], versionTokenSet)) < 0)
	{
		Logger::debug(LOG_LINE) << "Protocol version is not valid";
		throw(400);
	}
	httpVersion = tokenset[1];
}

void RequestParser::fillHeaderBuffer(std::map<std::string, std::string> &headerbuf,
																		 const std::string &line,
																		 const size_t headerbufSize)
{
	std::string headerToken;
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
	{
		Logger::debug(LOG_LINE) << "Header secion size is too long";
		throw(431); // rfc6585.5
	}
	headerToken = trimStr(const_cast<std::string &>(line), "\r\n");
	tmp = splitStr(headerToken, ":");
	if (tmp.size() < 2 || RequestParser::checkHeaderFieldnameHasSpace(tmp[0]))
	{
		Logger::debug(LOG_LINE) << "Header field has space || Header field does not have field-value";
		throw(400);
	}
	if (tmp[0].length() > RequestParser::maxHeaderFieldSize)
	{
		Logger::debug(LOG_LINE) << "Header field-name is too long";
		throw(431);
	}
	tmp[0] = tolowerStr(tmp[0].c_str());
	tmp[1] = line;
	tmp[1] = trimStr(tmp[1].erase(0, tmp[0].size() + 1), " ");
	if (RequestParser::checkHeaderFieldContain(headerbuf, tmp[0]))
	{
		tmpstring = trimStr(tmp[1], " ") + std::string(", ");
		headerbuf[tmp[0]] = tmpstring + headerbuf[tmp[0]];
	}
	else
	{
		headerbuf[tmp[0]] = trimStr(tmp[1], " ");
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
#if DEBUG > 1
	std::cout << "header section is done\n";
#endif
	std::vector<FieldValue> fieldvalueVec;

	if (!RequestParser::checkHeaderFieldContain(headerbuf, "Host"))
	{
		Logger::debug(LOG_LINE) << "Header Section does not contain 'Host' field";
		throw(400);
	}
	if (RequestParser::checkHeaderFieldContain(headerbuf, "Transfer-Encoding") &&
			::strstr(headerbuf[tolowerStr("Transfer-Encoding")].c_str(), "chunked"))
	{
		if (RequestParser::checkHeaderFieldContain(headerbuf, "Content-Length"))
			headerbuf[tolowerStr("Transfer-Encoding")] = "";
	}
	else
	{
		if (!RequestParser::checkHeaderFieldContain(headerbuf, "Content-Length"))
		{
#if DEBUG
			std::cout << "no chunked and content-length\n";
#endif
			if (!(method == "GET" || method == "HEAD"))
			{
				Logger::debug(LOG_LINE) << "Length is required";
				throw(411);
			} // length required error
		}
	}
	for (std::map<std::string, std::string>::const_iterator it = headerbuf.begin();
			 it != headerbuf.end(); it++)
	{
		RequestParser::headerValueParser(fieldvalueVec, it->second);
		header.headerMap[tolowerStr(it->first.c_str())] = fieldvalueVec;
		fieldvalueVec.clear();
	}
	if (!RequestParser::validateHeaderField(header))
	{
		Logger::debug(LOG_LINE) << "Some Header does not have valid token or format";
		throw(400);
	}

#if DEBUG > 1
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
		descriptionTokenSet = splitStr(trimStr(valueTokenSet[i], " "), ";");
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
		pairTokenSet = splitStr(trimStr(descriptionTokenSet[i], " "), "=");
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

bool RequestParser::validateHeaderField(Header &header)
{
	/**
	 * 값이 여러개일 때, 모두 같은 값이 와야 하는 필드: content-length, host, connection, User-Agent, Date
	 * content-length: 10진수의 문자만
	 * transfer-encoding: chunked, compress, deflate, gzip 중 하나
	 * connection: keep-alive, close 둘중 하나만
	 */
	for (std::map<std::string, std::vector<FieldValue> >::iterator it = header.headerMap.begin();
			 it != header.headerMap.end(); it++)
	{
		// RequestParser::checkAllValueIsSame();

		if (it->first == tolowerStr("Content-Length"))
		{
			//check isdigit
		}
		else if (it->first == tolowerStr("connection"))
		{
			// check token
		}
		else if (it->first == tolowerStr("Transfer-Encoding"))
		{
			// check token
		}
	}
	return true;
}

bool RequestParser::checkHeaderFieldnameHasSpace(const std::string &fieldname)
{
	return (fieldname.find(' ') != std::string::npos);
}

bool RequestParser::checkHeaderFieldContain(std::map<std::string, std::string> &headerbuf,
																						const std::string fieldname)
{
	return (headerbuf[tolowerStr(fieldname.c_str())].size() > 0);
}

/**
 * For Body Parser
 */
ssize_t RequestParser::bodyParser(Body &body, std::vector<char> &bodyOctets, Header &header)
{
	if (header.headerMap[tolowerStr("Transfer-Encoding")].size())
	{
		std::vector<FieldValue> values = header.headerMap[tolowerStr("Transfer-Encoding")];
		for (size_t i = 0; i < values.size(); i++)
		{
			if (values[i].value == "chunked")
				return RequestParser::chunkedBodyParser(body, bodyOctets);
		}
	}
	else if (header.headerMap[tolowerStr("Content-Length")].size())
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

void RequestParser::postBodyParser(Body &body, Header &header)
{
	std::vector<FieldValue> fieldValueVec = header.headerMap[tolowerStr("Content-Type")];
	std::string boundary;
	bool isMultipart;

	isMultipart = false;
	for (size_t i = 0; i < fieldValueVec.size(); i++)
	{
		Logger::debug(LOG_LINE) << "boundary is required in multipart/form-data body";
		throw(400);
	}
	if (!isMultipart)
		return;
	RequestParser::parseMultipartBody(body, boundary);
}

void RequestParser::parseMultipartBody(Body &body, const std::string &boundary)
{
#if DEBUG > 1
	std::cout << "[ " << __FUNCTION__ << " ]" << std::endl;
#endif
	std::vector<std::string> bodySet;
	std::string rawdata;

	rawdata.append(body.payload.begin(), body.payload.end());
	bodySet = splitStrStrict(rawdata, boundary.c_str(), boundary.length());
	for (size_t i = 0; i < bodySet.size() - 1; i++)
	{
		RequestParser::parseMultipartEachBody(body, trimStr(bodySet[i], "\r\n"));
	}
}

void RequestParser::parseMultipartEachBody(Body &body, const std::string &eachBody)
{
#if DEBUG > 1
	std::cout << "[ " << __FUNCTION__ << " ]" << std::endl;
#endif
	std::map<std::string, std::string> headerbuf;
	std::vector<std::string> sectionSet;
	std::stringstream ss;
	std::string line;
	Header eachHeader;

	sectionSet = splitStrStrict(eachBody, "\r\n\r\n", 4);
	if (sectionSet.size() == 1)
	{
		body.multipartFormData.push_back(
				std::make_pair(Header(), std::vector<char>(sectionSet[0].begin(), sectionSet[0].end())));
		return;
	}
	if (sectionSet.size() != 2)
	{
		Logger::debug(LOG_LINE)
				<< "Multipart format body does not have header-body pair || section delimiter is invalid";
		throw(400);
	}
	ss << sectionSet[0]; // header는 binary data가 아니므로 string stream으로 관리해도 됨
	while (std::getline(ss, line))
	{
		RequestParser::fillHeaderBuffer(headerbuf, line, 0);
		if (ss.eof())
			break;
	}
	const std::string dummyMethod = "GET";
	const std::string dummyHostField = "localhost";
	headerbuf[tolowerStr("Host")] = dummyHostField;
	RequestParser::headerParser(eachHeader, headerbuf, dummyMethod);
#if DEBUG > 2
	eachHeader.print();
#endif
	body.multipartFormData.push_back(
			std::make_pair(eachHeader, std::vector<char>(sectionSet[1].begin(), sectionSet[1].end())));
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
			if ((lineLength = RequestParser::parseChunkedLengthLine(body, bodyOctets, lineBuffer)) < 0)
				break;
			if (lineLength == 0)
			{
				body.setParseFlag(Body::FINISHED);
				break;
			}
			body.setParseFlag(Body::CHUNKED_CONTENT);
		}
		else if (body.getParseFlag() == Body::CHUNKED_CONTENT)
		{
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
				length = ::strtol(std::string(lineBuffer.begin(), lineBuffer.end()).c_str(), NULL, 16);
				lineBuffer.clear();
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
			length = ::strtol(std::string(lineBuffer.begin(), lineBuffer.end()).c_str(), NULL, 16);
			lineBuffer.clear();
			break;
		}
		if (!::isxdigit(bodyOctets[i]))
		{
			Logger::debug(LOG_LINE) << "Length in chunked body does not hexadecimal number";
			throw(400);
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

	targetSize = ::strtol(header.headerMap[tolowerStr("Content-Length")][0].value.c_str(), NULL, 10);
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

std::vector<std::string>
RequestParser::splitStrStrict(const std::string &str, const char *delimiter, size_t delimiterSize)
{
	std::vector<std::string> tokenset;
	size_t pos;
	size_t idx;

	pos = 0;
	idx = 0;
	while (1)
	{
		if (pos >= str.length())
			break;
		idx = str.find(delimiter, pos);
		if (idx == std::string::npos)
		{
			tokenset.push_back(str.substr(pos, str.length() - pos));
			break;
		}
		if (idx == 0)
		{
			pos = idx + delimiterSize;
			continue;
		}
		tokenset.push_back(str.substr(pos, idx - pos));
		pos = idx + delimiterSize;
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

// std::string &RequestParser::trimStrStrict(std::string &target, const std::string &charset)
// {
// 	size_t idx;
// 	std::string::iterator it;

// 	idx = 0;
// 	while (idx < target.size())
// 	{
// 		it = target.begin();
// 		if (charset.find(target[idx]) == std::string::npos)
// 			break;
// 		target.erase(it + idx);
// 	}

// 	idx = target.size() - 1;
// 	while (idx >= 0)
// 	{
// 		it = target.begin();
// 		if (charset.find(target[idx]) == std::string::npos)
// 			break;
// 		target.erase(it + idx);
// 		--idx;
// 	}
// 	return target;
// }

std::string RequestParser::tolowerStr(const char *str)
{
	std::string copystr;

	copystr = str;
	std::transform(copystr.begin(), copystr.end(), copystr.begin(), ::tolower);
	return copystr;
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
