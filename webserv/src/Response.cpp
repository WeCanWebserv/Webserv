#include "Response.hpp"
#include <fcntl.h>
#include <unistd.h>

// FIX: 매크로는 최대한 제거
#define SERVER_NAME "webserv"
#define SERVER_PROTOCOL "HTTP/1.1"
#define CRLF "\r\n"

namespace ft
{
template<class T>
std::string toString(T value)
{
	std::stringstream ss;

	ss << value;
	return (ss.str());
}
} // namespace ft

Response::statusInfoType Response::defaultInfo = initializeDefaultInfo();

Response::Response() : statusCode(200), sentBytes(0), totalBytes(0), isReady(false)
{
	this->header["Server"] = SERVER_NAME;
	// FIX: get current date / rfc5322
	this->header["Date"] = "Tue, 23 Aug 2022 15:00:00 GMT";
}

Response::Response(const Response &other)
		: statusCode(other.statusCode), header(other.header), body(), buffer(other.buffer),
			sentBytes(0), totalBytes(other.totalBytes), isReady(other.isReady)
{}

Response::~Response() {}

bool Response::ready() const
{
	return (this->isReady);
}

bool Response::done() const
{
	return (ready() && this->sentBytes == this->totalBytes);
}

void Response::clear()
{
	buffer.clear();
	totalBytes = 0;
	sentBytes = 0;

	clearBody(this->body);

	header.clear();
	header["Server"] = SERVER_NAME;

	isReady = false;
}

const char *Response::getBuffer() const
{
	return (this->buffer.c_str() + this->sentBytes);
}

std::size_t Response::getBufSize() const
{
	return (this->totalBytes - this->sentBytes);
}

std::size_t Response::moveBufPosition(int nbyte)
{
	if (nbyte > 0)
	{
		if (this->sentBytes + nbyte > this->totalBytes)
			this->sentBytes = this->totalBytes;
		else
			this->sentBytes += nbyte;
	}
	return (getBufSize());
}

template<class Request, class ConfigInfo>
void Response::setRequest(Request &req, ConfigInfo config)
{
	Uri uri;
	std::string targetPath;
	std::string loc;
	typename ConfigInfo::locationType *location;

	uri = createUri(req.uri);
	// TODO
	loc = findLocation(uri, config.location);
	location = &config.location[loc];
	if (location == NULL)
		throw (404);
	if (location->allowMethod.find(req.method) == location->allowMethod.end())
		throw (405);

	if (location->cgis.find(uri.extension) != location->cgis.end())
		throw (501); // cgi.

	if (uri.isDirectory)
		throw (501); // find indexFile or directory listing.

	targetPath = uri.path;
	targetPath = targetPath.replace(0, loc.size(), location->root);

	this->body.fd = open(targetPath.c_str(), O_RDONLY);
	if (this->body.fd == -1)
		throw (404);
}

template<class ConfigInfo>
void Response::setError(int code, ConfigInfo config, bool close)
{
	std::string errorPage;

	clear();

	this->statusCode = code;

	if (close)
	{
		this->isClose = close;
		setHeader("Connection", "close");
	}

	if (config.errorPages.find(code) != config.errorPages.end())
	{
		errorPage = config.errorPages[code];
		this->body.fd = open(errorPage.c_str(), O_RDONLY);
		if (this->body.fd == -1)
			setBodyToDefaultErrorPage(code);
	}
	else
		setBodyToDefaultErrorPage(code);
}

void Response::setBodyToDefaultErrorPage(int code)
{
	std::string errorPage;

	errorPage = generateDefaultErrorPage(code);
	this->body.buffer << errorPage;
	this->body.size = errorPage.size();
	setHeader("Content-Length", ft::toString(this->body.size));
	setHeader("Content-Type", "text/html");
	setBuffer();
}

std::string Response::generateDefaultErrorPage(int code)
{
	std::stringstream html;
	std::string errorMsg;

	errorMsg = getStatusInfo(code);

	html << "<html>";
	html << "<head>"
			 << "<title>" << errorMsg << "</title>"
			 << "</head>";
	html << "<body>"
			 << "<h1>" << code << " " << errorMsg << "</h1>"
			 << "</body>";
	html << "</html>";
	return (html.str());
}

int Response::readBody()
{
	const std::size_t bufSize = 4096 * 16;
	char buf[bufSize];
	int n;

	if (this->body.fd == -1)
		return (-1);
	n = read(this->body.fd, buf, bufSize - 1);
	if (n == -1)
		return (-1);
	else if (n == 0)
	{
		setHeader("Content-Length", ft::toString(this->body.size));
		// FIX: {extension: media-type} 의 형태로 지원하는 타입 정의하기
		setHeader("Content-Type", "text/html");
		setBuffer();
		return (0);
	}
	buf[n] = '\0';
	this->body.buffer << buf;
	this->body.size += n;
	return (1);
}

void Response::setStatusCode(int code)
{
	this->statusCode = code;
}

void Response::setHeader(std::string name, std::string value)
{
	this->header[name] = value;
}

void Response::setBuffer()
{
	std::stringstream tmp;

	tmp << SERVER_PROTOCOL << " " << this->statusCode << " " << getStatusInfo(this->statusCode)
			<< CRLF;

	for (headerType::const_iterator it = this->header.begin(), ite = this->header.end(); it != ite;
			 ++it)
	{
		tmp << (*it).first << ": " << (*it).second << CRLF;
	}
	tmp << CRLF;

	if (this->statusCode / 100 != 1 && this->statusCode != 204 && this->statusCode != 304)
	{
		if (this->body.buffer.rdbuf()->in_avail())
			tmp << this->body.buffer.rdbuf();
	}

	this->buffer = tmp.str();
	this->totalBytes = this->buffer.size();
	this->isReady = true;
}

Response::statusInfoType Response::initializeDefaultInfo()
{
	statusInfoType info;

	info[100] = "Continue";
	info[101] = "Switching Protocols";
	info[200] = "OK";
	info[201] = "Created";
	info[202] = "Accepted";
	info[203] = "Non-Authoritative Information";
	info[204] = "No Content";
	info[205] = "Reset Content";
	info[206] = "Partial Content";
	info[300] = "Multiple Choices";
	info[301] = "Moved Permanently";
	info[302] = "Found";
	info[303] = "See Other";
	info[304] = "Not Modified";
	info[305] = "Use Proxy";
	info[307] = "Temporary Redirect";
	info[400] = "Bad Request";
	info[401] = "Unauthorized";
	info[402] = "Payment Required";
	info[403] = "Forbidden";
	info[404] = "Not Found";
	info[405] = "Method Not Allowd";
	info[406] = "Not Acceptable";
	info[407] = "Proxy Authentication Required";
	info[408] = "Request Timeout";
	info[409] = "Conflict";
	info[410] = "Gone";
	info[411] = "Length Required";
	info[412] = "Precondition Failed";
	info[413] = "Payload Too Large";
	info[414] = "URI Too Long";
	info[415] = "Unsupported Media Type";
	info[416] = "Range Not Satisfiable";
	info[417] = "Expectation Failed";
	info[426] = "Upgrade Required";
	info[500] = "Internal Server Error";
	info[501] = "Not Implemented";
	info[502] = "Bad Gateway";
	info[503] = "Service Unavailable";
	info[504] = "Gateway Timeout";
	info[505] = "HTTP Version Not Supported";
	return (info);
}

std::string Response::getStatusInfo(int code) const
{
	if (code < 100 || code >= 600)
		return ("Undefined Status");

	if (this->defaultInfo.find(code) == this->defaultInfo.end())
		code = (code / 100) * 100; // e.g. 2xx -> 200

	return (this->defaultInfo[code]);
}

Response::Uri Response::createUri(const std::string &originUri)
{
	Uri uri;
	std::string::size_type pos;

	if (originUri.size() > 0)
	{
		uri.originUri = originUri;

 		// if absolute-uri
		pos = originUri.find("://");
		if (originUri[0] != '/' && pos != std::string::npos)
		{
			pos = originUri.find('/', pos + 1);
			uri.path = pos == std::string::npos ? "/" : originUri.substr(pos);
		}
		else
		{
			pos = originUri.find_first_not_of(".");
			uri.path = pos == std::string::npos ? "/" : originUri.substr(pos);
		}

		// if has query
		pos = uri.path.find("?");
		if (pos != std::string::npos)
		{
			uri.query = uri.path.substr(pos + 1, uri.path.find("#", pos));
			uri.path = uri.path.substr(0, pos);
		}

		// if has extension
		pos = uri.path.rfind(".");
		if (pos != std::string::npos)
		{
			uri.extension = uri.path.substr(pos);
		}
	}
	return (uri);
}

void Response::clearBody(Body &targetBody)
{
	targetBody.fd = -1;
	targetBody.size = 0;
	targetBody.buffer.str("");
}
