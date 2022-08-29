#include "Response.hpp"
#include "MediaType.hpp"
#include "ReasonPhrase.hpp"
#include "UriParser.hpp"
#include <ctime>
#include <fcntl.h>
#include <unistd.h>

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

Response::Response() : statusCode(200), body(), sentBytes(0), totalBytes(0), isReady(false) {}

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
	this->buffer.clear();
	this->totalBytes = 0;
	this->sentBytes = 0;

	clearBody(this->body);

	this->header.clear();

	this->isReady = false;
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
void Response::process(Request &req, ConfigInfo &config)
{
	UriParser uriParser(req.uri);
	std::string targetPath;
	typename ConfigInfo::locationType::iterator locIter;

	locIter = findLocation(uriParser.getPath(), config.location);
	if (locIter == config.location.end())
		throw(404);

	const std::string &locPath = (*locIter).first;
	typename ConfigInfo::locationType::mapped_type &location = (*locIter).second;

	if (location.allowMethod.find(req.method) == location.allowMethod.end())
		throw(405);

	if (location.cgis.find(uriParser.getExtension()) != location.cgis.end())
		throw(501); // cgi.

	if (uriParser.isDirectory())
		throw(501); // find indexFile or directory listing.

	targetPath = uriParser.getPath();
	targetPath.replace(0, locPath.size(), location.root);

	this->body.fd = open(targetPath.c_str(), O_RDONLY);
	if (this->body.fd == -1)
		throw(404);
	setHeader("Content-Type", MediaType::get(uriParser.getExtension()));
}

template<class ConfigInfo>
void Response::process(int errorCode, ConfigInfo &config, bool close)
{
	std::string errorPage;

	clear();

	this->statusCode = errorCode;

	if (close)
	{
		this->isClose = close;
		setHeader("Connection", "close");
	}

	if (config.errorPages.find(errorCode) != config.errorPages.end())
	{
		errorPage = config.errorPages[errorCode];
		this->body.fd = open(errorPage.c_str(), O_RDONLY);
		if (this->body.fd == -1)
			setBodyToDefaultErrorPage(errorCode);
		setHeader("Content-Type", MediaType::get(UriParser(errorPage).getExtension()));
	}
	else
		setBodyToDefaultErrorPage(errorCode);
}

void Response::setBodyToDefaultErrorPage(int code)
{
	std::string errorPage;

	errorPage = generateDefaultErrorPage(code);
	this->body.buffer << errorPage;
	this->body.size = errorPage.size();
	setHeader("Content-Length", ft::toString(this->body.size));
	setHeader("Content-Type", MediaType::get(".html"));
	setBuffer();
}

std::string Response::generateDefaultErrorPage(int code) const
{
	std::stringstream html;
	std::string errorMsg;

	errorMsg = ReasonPhrase::get(code);

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

	n = read(this->body.fd, buf, bufSize - 1);
	if (n == -1)
		return (-1);
	else if (n == 0)
	{
		setHeader("Content-Length", ft::toString(this->body.size));
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

	tmp << SERVER_PROTOCOL << " " << this->statusCode << " " << ReasonPhrase::get(this->statusCode)
			<< CRLF;

	tmp << "Server: " << SERVER_NAME << CRLF;
	tmp << "Date: " << getCurrentTime() << CRLF;
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

std::string Response::getCurrentTime() const
{
	const int bufSize = 32;
	char buf[bufSize];
	std::string format;
	std::time_t rawTime;
	std::tm *timeInfo;

	format = "%a, %d %b %G %T GMT";
	std::time(&rawTime);
	timeInfo = std::gmtime(&rawTime);
	std::strftime(buf, bufSize, format.c_str(), timeInfo);
	return (std::string(buf));
}

template<class Locations>
typename Locations::iterator Response::findLocation(std::string path, Locations &location)
{
	typename Locations::iterator found;
	std::string loc;
	std::string::size_type pos;

	while (path.size() > 0)
	{
		pos = path.rfind("/");
		if (pos == std::string::npos)
			break;
		loc = path.substr(0, pos + 1);
		found = location.find(loc);
		if (found != location.end())
			return (found);
		path = path.substr(0, pos);
	}
	return (location.end());
}

void Response::clearBody(Body &targetBody)
{
	targetBody.fd = -1;
	targetBody.size = 0;
	targetBody.buffer.str("");
}
