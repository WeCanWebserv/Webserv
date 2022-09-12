#ifndef __FT_REQUEST_H__
#define __FT_REQUEST_H__

#include "body.hpp"
#include "header.hpp"
#include "startline.hpp"

class Request
{
private:
	Startline startline;
	Header header;
	Body body;

public:
	Request();
	// ~Request();
	Startline &getStartline(void);
	Header &getHeader(void);
	Body &getBody(void);
};

#endif
