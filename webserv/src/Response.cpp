#include "Response.hpp"
#include <fcntl.h>

// FIX: 매크로는 최대한 제거
#define SERVER_NAME "webserv"
#define SERVER_PROTOCOL "HTTP/1.1"
#define CRLF "\r\n"

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
	return (this->sentBytes == this->totalBytes);
}

void Response::clear()
{
	// FIX: 비용이 비싸지 않을까 우려됨
	buffer.clear();
	totalBytes = 0;
	sentBytes = 0;

	body.clear();

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
int Response::setRequest(Request &req, ConfigInfo config)
{
	Uri uri(req.uri);
	std::string targetPath;
	std::string loc;
	typename ConfigInfo::locationType *location;

	loc = findLocation(uri, config.location);
	location = &config.location[loc];
	if (location == NULL)
		return (setError(404, config));
	if (location->allowMethod.find(req.method) == location->allowMethod.end())
		return (setError(405, config));

	if (location->cgis.find(uri.extension) != location->cgis.end())
		return (setError(501, config, true)); // cgi.

	if (uri.isDirectory)
		return (setError(501, config, true)); // find indexFile or directory listing.

	targetPath = uri.path.replace(0, loc.size(), location->root);

	this->body.clear();
	this->body.fd = open(targetPath.c_str(), O_RDONLY);
	if (this->body.fd == -1)
		return (-1);
	return (0);
}

template<class ConfigInfo>
int Response::setError(int code, ConfigInfo config, bool close)
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
		errorPage = config.errorPages[code];
	else
		errorPage = getDefaultErrorPage(code);

	// open errorPage
	this->body.fd = open(errorPage.c_str(), O_RDONLY);
	if (this->body.fd == -1)
		return (-1);
	return (0);
}

std::stringstream &Response::getBodyStream()
{
	return (this->body.buffer);
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
