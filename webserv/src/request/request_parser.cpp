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
#include <iostream>
#endif

std::vector<std::string> RequestParser::methodTokenSet = RequestParser::initMethodTokenSet();

void RequestParser::startlineParser(Startline &startline, const std::string &line)
{
	Logger::debug(LOG_LINE) << "Startline section is done\n";

	std::vector<std::string> startlineTokenSet;

	startlineTokenSet = splitStr(line, " ");
	if (startlineTokenSet.size() != 3)
	{
		Logger::debug(LOG_LINE) << "Startline token count is not 3\n";
		throw(400);
	}

	RequestParser::startlineMethodParser(startline.method, startlineTokenSet[0]);
	RequestParser::startlineURIParser(startline.uri, startlineTokenSet[1], startline.method);
	RequestParser::startlineHTTPVersionParser(startline.httpVersion,
																						trimStr(startlineTokenSet[2], "\r"));
	std::string str;

	str.append("\n################### [ DEBUG: Startline print ] ##################\n\n");
	str.append("\n[ Parsed Startline Info ]\nMethod: ");
	str.append(startline.method);
	str.append("\nURI: ");
	str.append(startline.uri);
	str.append("\nHTTP Version: ");
	str.append(startline.httpVersion);
	str.append("\n##########################################################\n");
	Logger::debug(LOG_LINE) << str << "\n";
}

void RequestParser::startlineMethodParser(std::string &method, const std::string &token)
{
	ssize_t idx;

	/**
	 *  case-sensitive로 확인해야 한다.
	 */
	if ((idx = findToken(token, RequestParser::methodTokenSet)) < 0)
	{
		Logger::debug(LOG_LINE) << "No match method in Method Tokens\n";
		throw(403);
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
		Logger::debug(LOG_LINE) << "No matched uri form\n";
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

	tokenset = splitStr(token, "/");
	if (tokenset[0].compare("HTTP"))
	{
		Logger::debug(LOG_LINE) << "Protocol is not HTTP\n";
		throw(400);
	}
	versionTokenSet = splitStr(versionList, ", ");
	if ((idx = findToken(tokenset[1], versionTokenSet)) < 0)
	{
		Logger::debug(LOG_LINE) << "Protocol version is not valid\n";
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

	std::stringstream ss;
	ss << headerbuf.size();
	Logger::debug(LOG_LINE) << "header line [" << ss.str() << "] : " << line << "\n";
	if (headerbufSize + line.length() + 1 > RequestParser::maxHeaderSize)
	{
		Logger::debug(LOG_LINE) << "Header secion size is too long\n";
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
		Logger::debug(LOG_LINE) << "Header field-name is too long\n";
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
}

void RequestParser::headerParser(Header &header,
																 std::map<std::string, std::string> &headerbuf,
																 const std::string &method,
																 size_t maxBodySize)
{
	Logger::debug(LOG_LINE) << "Header section is done\n";
	std::vector<FieldValue> fieldvalueVec;

	if (!RequestParser::checkHeaderFieldContain(headerbuf, "Host"))
	{
		Logger::debug(LOG_LINE) << "Header Section does not contain 'Host' field\n";
		throw(400);
	}
	if (RequestParser::checkHeaderFieldContain(headerbuf, TRANSFER_ENCODING) &&
			::strstr(headerbuf[tolowerStr(TRANSFER_ENCODING)].c_str(), "chunked"))
	{
		if (RequestParser::checkHeaderFieldContain(headerbuf, CONTENT_LENGTH))
			headerbuf[tolowerStr(CONTENT_LENGTH)] = "";
	}
	else
	{
		if (!RequestParser::checkHeaderFieldContain(headerbuf, CONTENT_LENGTH))
		{
			if (!(method == "GET" || method == "DELETE"))
			{
				Logger::debug(LOG_LINE) << "Length is required\n";
				throw(411); // length required error
			}
		}
	}
	for (std::map<std::string, std::string>::const_iterator it = headerbuf.begin();
			 it != headerbuf.end(); it++)
	{
		RequestParser::headerValueParser(fieldvalueVec, it->second);
		Logger::debug(LOG_LINE) << "fieldname : " << it->first << "\n";
		header.insertField(std::make_pair(it->first, fieldvalueVec));
		fieldvalueVec.clear();
	}
	if (!RequestParser::validateHeaderField(header, maxBodySize))
	{
		Logger::debug(LOG_LINE) << "Some Header does not have valid token or format\n";
		throw(400);
	}
	header.print();
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
		fieldvalue.value = trimStr(descriptionTokenSet[0], "\r\n");
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
			descriptions.insert(std::make_pair(pairTokenSet[0], trimStr(pairTokenSet[1], "\r\n\"")));
		else
		{
			std::string tmp;
			for (size_t j = 1; j < pairTokenSet.size(); j++)
				tmp = tmp + pairTokenSet[i];
			descriptions.insert(std::make_pair(pairTokenSet[0], trimStr(tmp, "\r\n\"")));
		}
	}
}

bool RequestParser::validateHeaderField(Header &header, size_t maxBodySize)
{
	/**
	 * 값이 여러개일 때, 모두 같은 값이 와야 하는 필드: content-length, host, connection, User-Agent, Date
	 * content-length: 10진수의 문자만
	 * transfer-encoding: chunked, compress, deflate, gzip 중 하나
	 * connection: keep-alive, close 둘중 하나만
	 */
	for (std::map<std::string, std::vector<FieldValue> >::const_iterator it =
					 header.getFields().begin();
			 it != header.getFields().end(); it++)
	{
		// RequestParser::checkAllValueIsSame();
		// TODO: Validation 마저 하기
		if (it->first == tolowerStr(CONTENT_LENGTH))
		{
			std::stringstream tmp(it->second[0].value);
			size_t x = 0;
			tmp >> x;
			if (x > maxBodySize)
			{
				Logger::debug(LOG_LINE) << "Payload is Too Long\n";
				Logger::debug(LOG_LINE) << "maxBodySize: " << maxBodySize << ", content-length size: " << x << "\n";
				throw(413);
			} // payload too long
				//check isdigit
		}
		else if (it->first == tolowerStr("connection"))
		{
			// check token
		}
		else if (it->first == tolowerStr(TRANSFER_ENCODING))
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
	return (headerbuf.count(tolowerStr(fieldname.c_str())) > 0);
}

/**
 * For Body Parser
 */
ssize_t RequestParser::bodyParser(Body &body, std::vector<char> &bodyOctets, Header &header)
{
	if (bodyOctets.size())
		Logger::debug(LOG_LINE) << "Body Octet: " << std::string(bodyOctets.begin(), bodyOctets.end())
														<< std::endl;
	if (header.hasField(TRANSFER_ENCODING))
	{
		if (header.hasFieldValue(TRANSFER_ENCODING, "chunked"))
		{
			Logger::debug(LOG_LINE) << "Body Type: Chunked\n";
			return RequestParser::chunkedBodyParser(body, bodyOctets);
		}
	}
	else if (header.hasField(CONTENT_LENGTH))
	{
		Logger::debug(LOG_LINE) << "Body Type: Content-Length\n";
		return RequestParser::contentLengthBodyParser(body, bodyOctets, header);
	}
	/**
	 * GET이나 HEAD일 경우에 해당한다.
	 * 나머지 경우는 HeaderParser가 완성된 후로 validation을 완료 하였다.
	 */

	body.setParseFlag(Body::FINISHED);
	return (0);
}

void RequestParser::postBodyParser(Body &body, Header &header, size_t maxBodySize)
{
	FieldValue fieldvalue;
	std::map<std::string, std::string>::const_iterator boundary;

	fieldvalue = header.getFieldValue("Content-Type", "multipart/form-data");
	if (!fieldvalue.value.size())
	{
		body.print();
		return;
	}
	boundary = fieldvalue.descriptions.find("boundary");
	if (boundary == fieldvalue.descriptions.end())
	{
		Logger::debug(LOG_LINE) << "boundary is required in multipart/form-data body\n";
		throw(400);
	}
	RequestParser::parseMultipartBody(body, boundary->second, maxBodySize);
	body.print();
}

void RequestParser::parseMultipartBody(Body &body, const std::string &boundary, size_t maxBodySize)
{
	std::vector<std::string> bodySet;
	std::string rawdata;

	rawdata.append(body.payload.begin(), body.payload.end());
	bodySet = splitStrStrict(rawdata, boundary.c_str(), boundary.length());
	if (!bodySet.size())
		return;
	Logger::debug(LOG_LINE) << "This is Multipart Form-data Body\n";
	for (size_t i = 0; i < bodySet.size() - 1; i++)
		RequestParser::parseMultipartEachBody(body, trimStr(bodySet[i], "\r\n"), maxBodySize);
}

void RequestParser::parseMultipartEachBody(Body &body,
																					 const std::string &eachBody,
																					 size_t maxBodySize)
{
	std::vector<std::string> sectionSet;

	sectionSet = splitStrStrict(eachBody, "\r\n\r\n", 4);
	if (sectionSet.size() == 1)
	{
		body.multipartFormData.push_back(
				std::make_pair(Header(), std::vector<char>(sectionSet[0].begin(), sectionSet[0].end())));
		return;
	}
	RequestParser::parseMultipartEachHeader(body, sectionSet[0], sectionSet[1], maxBodySize);
}

void RequestParser::parseMultipartEachHeader(Body &body,
																						 const std::string &headerSection,
																						 const std::string &bodySection,
																						 size_t maxBodySize)
{
	const std::string dummyMethod = "GET";
	const std::string dummyHostField = "localhost";
	std::map<std::string, std::string> headerbuf;
	std::stringstream ss;
	std::string line;
	Header eachHeader;

	ss << headerSection; // header는 binary data가 아니므로 string stream으로 관리해도 됨
	while (std::getline(ss, line))
	{
		RequestParser::fillHeaderBuffer(headerbuf, line, 0);
		if (ss.eof())
			break;
	}
	headerbuf[tolowerStr("Host")] = dummyHostField;
	RequestParser::headerParser(eachHeader, headerbuf, dummyMethod, maxBodySize);
#if DEBUG > 1
	eachHeader.print();
#endif
	body.multipartFormData.push_back(
			std::make_pair(eachHeader, std::vector<char>(bodySection.begin(), bodySection.end())));
}

ssize_t RequestParser::chunkedBodyParser(Body &body, const std::vector<char> &bodyOctets)
{
	ssize_t lineLength;
	std::vector<char> lineBuffer;
	std::vector<char> copyBodyOctets = bodyOctets;

	body.chunkedParsingPrologue(lineBuffer, lineLength);

	while (copyBodyOctets.size())
	{
		if (body.getParseFlag() == Body::CHUNKED_LENGTH)
		{
			if ((lineLength = RequestParser::parseChunkedLengthLine(body, copyBodyOctets, lineBuffer)) <
					0)
			{
				// line length parsing 전에 입력이 먼저 끝남
				break;
			}
			if (lineLength == 0)
			{
				// chunked의 끝을 알리는 0이 들어온 것.
				body.setParseFlag(Body::FINISHED);
				break;
			}
			body.setParseFlag(Body::CHUNKED_CONTENT);
		}
		else if (body.getParseFlag() == Body::CHUNKED_CONTENT)
		{
			if (!RequestParser::parseChunkedContentLine(body, copyBodyOctets, lineBuffer, lineLength))
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
		Logger::debug(LOG_LINE) << "Body Parse is done\n";
		return (copyBodyOctets.size());
	}
	body.chunkedParsingEpilogue(lineBuffer, lineLength);
	return (-1); // 모자르다는 의미
}

ssize_t RequestParser::parseChunkedLengthLine(Body &body,
																							std::vector<char> &bodyOctets,
																							std::vector<char> &lineBuffer)
{
	size_t i;
	ssize_t length = -1;
	bool crFlag = false;

	for (i = 0; i < bodyOctets.size(); i++)
	{
		if (bodyOctets[i] == '\n')
		{
			if (lineBuffer[lineBuffer.size() - 1] != '\r') // 그 전 read할 때 딱 \r까지만 읽었을 경우
			{
				Logger::debug(LOG_LINE) << "Chunked body does not end with '\\r\\n'\n";
				throw(400);
			}
			lineBuffer.pop_back();
			bodyOctets.erase(bodyOctets.begin(), bodyOctets.begin() + (i + 1)); // end는 미포함이므로 +1
			body.setParseFlag(Body::CHUNKED_CONTENT);
			length = ::strtol(std::string(lineBuffer.begin(), lineBuffer.end()).c_str(), NULL, 16);
			lineBuffer.clear();
			break;
		}
		if ((!::isxdigit(bodyOctets[i]) && bodyOctets[i] != '\r') || (bodyOctets[i] == '\r' && crFlag))
		{
			Logger::debug(LOG_LINE) << "Length in chunked body does not hexadecimal number\n";
			throw(400);
		}
		lineBuffer.push_back(bodyOctets[i]);
		if (bodyOctets[i] == '\r')
			crFlag = true;
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

	for (i = 0; i < static_cast<ssize_t>(bodyOctets.size()); i++)
	{
		if (i + savedlength == lineLength + 2) // \r\n을 받기 위해
		{
			body.payload.insert(body.payload.end(), lineBuffer.begin(), lineBuffer.end() - 2);
			lineBuffer.clear();
			bodyOctets.erase(bodyOctets.begin(), bodyOctets.begin() + i);
			return (0);
		}
		lineBuffer.push_back(bodyOctets[i]);
	}
	bodyOctets.clear();
	return (i - (lineLength + 2));
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

	targetSize = ::strtol(header.getFieldValueList(CONTENT_LENGTH)[0].value.c_str(), NULL, 10);
	remainPayloadSize = targetSize - body.payload.size();
	inputPayloadSize = bodyOctets.size();
	if (remainPayloadSize >= inputPayloadSize)
	{
		body.payload.insert(body.payload.end(), bodyOctets.begin(), bodyOctets.end());
	}
	else
	{
		body.payload.insert(body.payload.end(), bodyOctets.begin(),
												bodyOctets.begin() + remainPayloadSize);
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
std::string RequestParser::trimStr(const std::string &target, const std::string &charset)
{
	ssize_t idx;
	std::string::iterator it;
	std::string copyTarget = target;

	idx = 0;
	while (idx < static_cast<ssize_t>(copyTarget.size()))
	{
		it = copyTarget.begin();
		if (charset.find(copyTarget[idx]) == std::string::npos)
			break;
		copyTarget.erase(it + idx);
	}

	idx = copyTarget.size() - 1;
	while (idx >= 0)
	{
		it = copyTarget.begin();
		if (charset.find(copyTarget[idx]) == std::string::npos)
			break;
		copyTarget.erase(it + idx);
		--idx;
	}
	return copyTarget;
}

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
	// tokenset.push_back("HEAD");
	// tokenset.push_back("PUT");
	// tokenset.push_back("PATCH");
	// tokenset.push_back("OPTIONS");
	// tokenset.push_back("CONNECT");
	// tokenset.push_back("TRACE");
	return tokenset;
}
