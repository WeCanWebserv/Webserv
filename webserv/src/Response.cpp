#include "Response.hpp"

#define SERVER_NAME "webserv"

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
