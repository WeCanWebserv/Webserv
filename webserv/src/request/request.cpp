#include "request.hpp"
#include "Logger.hpp"

Request::Request() {}
Request::Request(const Request &other)
{
	*this = other;
}
Request &Request::operator=(const Request &other)
{
	this->startline = other.startline;
	this->header = other.header;
	this->body = other.body;
	return (*this);
}

Startline &Request::getStartline(void)
{
	return this->startline;
}
Header &Request::getHeader(void)
{
	return this->header;
}
Body &Request::getBody(void)
{
	return this->body;
}