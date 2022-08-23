#include "Response.hpp"

#define SERVER_NAME "webserv"
#define CRLF "\r\n"

Response::statusInfoType Response::defaultInfo = initializeDefaultInfo();

Response::Response() : statusCode(200), sentBytes(0), totalBytes(0), isReady(false)
{
	this->header["Server"] = SERVER_NAME;
	// FIX: get current date / rfc5322
	this->header["Date"] = "Tue, 23 Aug 2022 15:00:00 GMT";
}

Response::Response(const Response &other)
		: statusCode(other.statusCode), header(other.header), body(other.body.str()),
			buffer(other.buffer), sentBytes(0), totalBytes(other.totalBytes), isReady(other.isReady)
{}

Response &Response::operator=(const Response &rhs)
{
	if (this != &rhs)
	{
		this->statusCode = rhs.statusCode;
		this->header = rhs.header;
		this->body.str(rhs.body.str());
		this->buffer = rhs.buffer;
		this->sentBytes = rhs.sentBytes;
		this->totalBytes = rhs.totalBytes;
		this->isReady = rhs.isReady;
	}
	return (*this);
}

Response::~Response() {}

bool Response::ready() const
{
	return (this->isReady);
}

bool Response::done() const
{
	return (this->sentBytes == this->totalBytes);
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

std::stringstream &Response::getBodyStream()
{
	return (this->body);
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

	tmp << "HTTP/1.1 " << statusCode << " " << getStatusInfo(statusCode) << CRLF;

	for (headerType::const_iterator it = header.begin(), ite = header.end(); it != ite; ++it)
	{
		tmp << (*it).first << ": " << (*it).second << CRLF;
	}
	tmp << CRLF;

	if (body.rdbuf()->in_avail())
		tmp << body.rdbuf();

	buffer = tmp.str();
	totalBytes = buffer.size();
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
	else if (defaultInfo.find(code) == defaultInfo.end())
		return (this->defaultInfo[code / 100]);
	return (this->defaultInfo[code]);
}
