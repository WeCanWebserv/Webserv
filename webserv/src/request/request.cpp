#include "request.hpp"
#include "../Logger.hpp"

Request::Request() {}

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