#include "Response.hpp"
#include "../request/body.hpp"
#include "MediaType.hpp"
#include "ReasonPhrase.hpp"
#include "UriParser.hpp"

#include <algorithm>
#include <ctime>
#include <utility>
#include <vector>

#include <dirent.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/stat.h>
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

Response::Response()
		: statusCode(200), body(), sentBytes(0), totalBytes(0), isReady(false), isClose(false)
{}

Response::Response(const Response &other)
		: statusCode(other.statusCode), header(other.header), body(), buffer(other.buffer),
			sentBytes(0), totalBytes(0), isReady(false), isClose(other.isClose)
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

bool Response::close() const
{
	return (this->isClose);
}

void Response::clear()
{
	clearBuffer();
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

/**
 * @returns: (fd, events)
 */
std::pair<int, int> Response::process(Request &req, const ServerConfig &config, int clientFd)
{
	UriParser uriParser(req.getStartline().uri);
	std::string targetPath = uriParser.getPath();
	std::map<std::string, LocationConfig>::const_iterator locIter;

	locIter = findLocation(targetPath, config.tableOfLocations);
	if (locIter == config.tableOfLocations.end())
		throw(404);

	const std::string &locPath = (*locIter).first;
	const LocationConfig &location = (*locIter).second;

	if (location.allowedMethods.find(req.getStartline().method) == location.allowedMethods.end())
		throw(405);

	targetPath.replace(0, locPath.size(), location.root);

	if (uriParser.isDirectory())
	{
		std::vector<std::string> files;

		files = readDirectory(targetPath);
		if (location.isAutoIndexOn)
			return (setBodyToDefaultPage(generateFileListPage(req.getStartline().uri, files)));
		else
		{
			std::string index;

			index = searchIndexFile(files, location.indexFiles);
			if (index.size() == 0)
				throw(404);
			targetPath += index;
		}
	}

	if (location.tableOfCgiBins.find(uriParser.getExtension()) != location.tableOfCgiBins.end())
	{
		cgi.run(req, config, location, clientFd);
		if (cgi.fail())
			throw(503);
		if (req.getBody().payload.size())
		{
			this->buffer = ::Body::vecToStr(req.getBody().payload);
			return (std::make_pair(cgi.fd[1], EPOLLOUT));
		}
		else
		{
			::close(cgi.fd[1]);
			this->body.fd = cgi.fd[0];
			return (std::make_pair(cgi.fd[0], EPOLLIN));
		}
	}

	this->body.fd = open(targetPath.c_str(), O_RDONLY | O_NONBLOCK);
	if (this->body.fd == -1)
		throw(404);
	setHeader("Content-Type", MediaType::get(UriParser(targetPath).getExtension()));
	return (std::make_pair(this->body.fd, EPOLLIN));
}

std::pair<int, int> Response::process(int errorCode, const ServerConfig &config, bool close)
{
	std::string errorPage;

	clear();

	this->statusCode = errorCode;

	if (close)
	{
		this->isClose = close;
		setHeader("Connection", "close");
	}

	if (config.tableOfErrorPages.find(errorCode) != config.tableOfErrorPages.end())
	{
		errorPage = config.tableOfErrorPages.at(errorCode);
		this->body.fd = open(errorPage.c_str(), O_RDONLY | O_NONBLOCK);
		if (this->body.fd != -1)
		{
			setHeader("Content-Type", MediaType::get(UriParser(errorPage).getExtension()));
			return (std::make_pair(this->body.fd, EPOLLIN));
		}
	}

	return (setBodyToDefaultPage(generateDefaultErrorPage(errorCode)));
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
		if (cgi)
			cgi.parseCgiResponse(*this);
		else if (this->body.size)
			setHeader("Content-Length", ft::toString(this->body.size));

		setBuffer();

		return (0);
	}
	buf[n] = '\0';
	this->body.buffer << buf;
	this->body.size += n;
	return (1);
}

int Response::writeBody()
{
	int n;

	n = write(this->cgi.fd[1], getBuffer(), getBufSize());
	if (n == -1)
		return (-1);
	else if (n == 0)
	{
		clearBuffer();
		return (this->cgi.fd[0]);
	}
	moveBufPosition(n);
	return (0);
}

std::map<std::string, LocationConfig>::const_iterator
Response::findLocation(std::string path, const std::map<std::string, LocationConfig> &location)
{
	std::map<std::string, LocationConfig>::const_iterator found;
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

	if (cgi)
	{
		tmp << this->body.buffer.rdbuf();
	}
	else
	{
		// end of header
		tmp << CRLF;

		if (this->statusCode / 100 != 1 && this->statusCode != 204 && this->statusCode != 304)
		{
			if (this->body.buffer.rdbuf()->in_avail())
				tmp << this->body.buffer.rdbuf();
		}
	}

	this->buffer = tmp.str();
	this->totalBytes = this->buffer.size();
	this->isReady = true;
}

std::pair<int, int> Response::setBodyToDefaultPage(const std::string &html)
{
	this->body.buffer << html;
	this->body.size = html.size();
	setHeader("Content-Length", ft::toString(this->body.size));
	setHeader("Content-Type", MediaType::get(".html"));
	setBuffer();
	return (std::make_pair(-1, 0));
}

std::string Response::generateDefaultErrorPage(int code) const
{
	std::stringstream html;
	std::string errorMsg;

	errorMsg = ReasonPhrase::get(code);

	html << "<html>\n";
	html << "<head><title>" << errorMsg << "</title></head>\n";
	html << "<body><h1>" << code << " " << errorMsg << "</h1></body>\n";
	html << "</html>\n";
	return (html.str());
}

std::string
Response::generateFileListPage(const std::string &path, const std::vector<std::string> &files) const
{
	std::stringstream html;
	std::size_t totalFiles = files.size();

	html << "<html>\n";
	html << "<head><title>Index of " << path << "</title></head>\n";
	html << "<body>\n";
	html << "<h1>Index of " << path << "</h1><hr><pre><a href=\"../\">../</a>\n";

	for (std::size_t i = 0; i < totalFiles; ++i)
	{
		// skip if dot file
		if (files[i][0] == '.')
			continue;

		struct stat st;

		if (stat(files[i].c_str(), &st) == -1)
			continue;
		// file name
		html << "<a href=\"" << files[i] << "\">" << files[i] << "</a>";
		html << std::string(50 - files[i].size(), ' ');

		// time of last status change
		html << timeInfoToString(std::gmtime(&st.st_ctime), "%a, %d %b %G %T");

		// total size
		std::string totalSize = ft::toString(st.st_size);
		html << std::string(20 - totalSize.size(), ' ') << totalSize << '\n';
	}

	html << "</pre><hr></body>\n";
	html << "</html>\n";
	return (html.str());
}

void Response::clearBuffer()
{
	this->buffer.clear();
	this->totalBytes = 0;
	this->sentBytes = 0;
}

void Response::clearBody(Body &targetBody)
{
	targetBody.fd = -1;
	targetBody.size = 0;
	targetBody.buffer.str("");
}

std::vector<std::string> Response::readDirectory(const std::string &path)
{
	std::vector<std::string> files;
	DIR *dir;

	dir = opendir(path.c_str());
	if (dir)
	{
		dirent *file;

		while ((file = readdir(dir)) != NULL)
		{
			files.push_back(file->d_name);
		}
		closedir(dir);
	}
	return (files);
}

std::string Response::searchIndexFile(const std::vector<std::string> &files,
																			const std::vector<std::string> &indexFiles)
{
	std::size_t indexSize;
	std::vector<std::string>::const_iterator found;

	indexSize = indexFiles.size();
	for (std::size_t i = 0; i < indexSize; ++i)
	{
		found = std::find(files.begin(), files.end(), indexFiles[i]);
		if (found != files.end())
			return (indexFiles[i]);
	}
	return ("");
}

std::string Response::timeInfoToString(std::tm *timeInfo, const std::string format) const
{
	const int bufSize = 32;
	char buf[bufSize];

	std::strftime(buf, bufSize, format.c_str(), timeInfo);
	return (std::string(buf));
}

std::string Response::getCurrentTime() const
{
	std::string format;
	std::time_t rawTime;
	std::tm *timeInfo;

	format = "%a, %d %b %G %T GMT";
	std::time(&rawTime);
	timeInfo = std::gmtime(&rawTime);
	return (timeInfoToString(timeInfo, format));
}
