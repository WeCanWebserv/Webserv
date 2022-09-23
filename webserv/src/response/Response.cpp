#include "Response.hpp"
#include "../libft.hpp"
#include "../request/request.hpp"
#include "MediaType.hpp"
#include "ReasonPhrase.hpp"
#include "Uri.hpp"


#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
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
	this->cgi.clear();

	this->statusCode = 200;
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
	Startline &startLine = req.getStartline();
	Uri uri(startLine.uri);
	std::map<std::string, LocationConfig>::const_iterator locIter;

	std::string conn = req.getHeader().getRawValue("connection");
	if (startLine.httpVersion == "HTTP/1.0" || ft::transform(conn, ::tolower) == "close")
		this->isClose = true;

	locIter = findLocation(uri.path, config.tableOfLocations);
	if (locIter == config.tableOfLocations.end())
	{
		Logger::info() << "Response::process: not found location block" << std::endl;
		throw(404);
	}

	const std::string &locPath = (*locIter).first;
	const LocationConfig &location = (*locIter).second;

	if (location.allowedMethods.find(startLine.method) == location.allowedMethods.end())
	{
		Logger::info() << "Response::process: not allowed method" << std::endl;
		throw(405);
	}

	if (location.isRedirectionSet)
	{
		return (setRedirect(location.redirectionSetting.first, location.redirectionSetting.second));
	}

	uri.setRoot(locPath, location.root);

	if (uri.path[uri.path.size() - 1] == '/')
	{
		std::vector<std::string> files;

		files = readDirectory(uri.getServerPath());
		if (location.isAutoIndexOn)
		{
			setBodyToDefaultPage(generateFileListPage(startLine.uri, location.root, files));
			return (std::make_pair(-1, -1));
		}
		else
		{
			std::string index;

			index = searchIndexFile(files, location.indexFiles);
			if (index.size() == 0)
			{
				Logger::info() << "Response::process: uri is directory and not found index file"
											 << std::endl;
				throw(404);
			}
			uri.setIndexFile(index);
		}
	}

	if (isDirectory(uri.getServerPath()))
	{
		return (setRedirect(301, uri.path + '/'));
	}

	this->body.file.open(uri.getServerPath().c_str());
	if (!this->body.file)
	{
		Logger::info() << "Response::process: open(" << uri.getServerPath()
									 << "): " << std::strerror(errno) << std::endl;
		throw(404);
	}

	if (location.tableOfCgiBins.find(uri.extension) != location.tableOfCgiBins.end())
	{
		this->body.file.close();
		std::string cgiBin = location.tableOfCgiBins.at(uri.extension);
		if (cgi.run(cgiBin, uri, req, config, clientFd))
		{
			Logger::error() << "Cgi::run: " << std::strerror(errno) << std::endl;
			throw(500);
		}
		if (cgi.fail())
			throw(503);

		if (req.getBody().payload.size())
		{
			this->buffer = ::Body::vecToStr(req.getBody().payload);
			this->totalBytes = req.getBody().payload.size();
			return (std::make_pair(cgi.fd[1], EPOLLOUT));
		}
		else
		{
			::close(cgi.fd[1]);
			this->body.fd = cgi.fd[0];
			return (std::make_pair(cgi.fd[0], EPOLLIN));
		}
	}

	Logger::debug(LOG_LINE) << "serve '" << uri.getServerPath() << "'" << std::endl;
	this->body.size = getFileSize();
	if (this->body.size)
	{
		setHeader("Content-Type", MediaType::get(uri.extension));
		setHeader("Content-Length", ft::toString(this->body.size));
		this->body.buffer << this->body.file.rdbuf();
	}
	setBuffer();
	return (std::make_pair(-1, -1));
}

void Response::process(int errorCode, const ServerConfig &config, bool close)
{
	clear();

	this->statusCode = errorCode;
	this->isClose |= close;

	if (config.tableOfErrorPages.find(errorCode) != config.tableOfErrorPages.end())
	{
		std::string errorPage = config.tableOfErrorPages.at(errorCode);

		Logger::debug(LOG_LINE) << "found error page: " << errorPage << std::endl;
		this->body.file.open(errorPage.c_str());
		if (this->body.file)
		{
			this->body.size = getFileSize();
			if (this->body.size)
			{
				setHeader("Content-Type", MediaType::get(Uri(errorPage).extension));
				setHeader("Content-Length", ft::toString(this->body.size));
				this->body.buffer << this->body.file.rdbuf();
			}
			setBuffer();
			return;
		}
	}

	setBodyToDefaultPage(generateDefaultErrorPage(errorCode));
}

int Response::readPipe()
{
	const std::size_t bufSize = 4096 * 16;
	char buf[bufSize];
	int n;

	n = read(this->cgi.fd[0], buf, bufSize - 1);
	if (n == -1)
	{
		Logger::error() << "Response::readBody: read: " << std::strerror(errno) << std::endl;
		return (-1);
	}
	buf[n] = '\0';
	this->body.buffer << buf;
	this->body.size += n;
	return (1);
}

int Response::writePipe()
{
	int n;

	n = write(this->cgi.fd[1], getBuffer(), getBufSize());
	if (n == -1)
	{
		Logger::error() << "Response::writeBody: write: " << std::strerror(errno) << std::endl;
		return (-1);
	}
	else if (n == 0)
	{
		clearBuffer();
		return (this->cgi.fd[0]);
	}
	moveBufPosition(n);
	return (0);
}

void Response::parseCgiResponse()
{
	int exitCode = this->cgi.exitCode();

	this->cgi.parseCgiResponse(*this);

	if (exitCode != 0 && this->body.buffer.rdbuf()->in_avail() == 0)
		throw(503);
	else if (this->statusCode == 200 && this->header.find("Content-Length") == this->header.end())
	{
		setStatusCode(204);
		this->body.buffer << CRLF;
	}
	setBuffer();
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

	if (isClose)
		setHeader("Connection", "close");
	else
		setHeader("Connection", "keep-alive");

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

std::pair<int, int> Response::setRedirect(int status, const std::string &location)
{
	clear();
	setStatusCode(status);
	setHeader("Location", location);
	setBuffer();
	return (std::make_pair(-1, -1));
}

std::pair<int, int> Response::setBodyToDefaultPage(const std::string &html)
{
	this->body.buffer << html;
	this->body.size = html.size();
	if (this->body.size)
	{
		setHeader("Content-Length", ft::toString(this->body.size));
		setHeader("Content-Type", MediaType::get(".html"));
	}
	setBuffer();
	return (std::make_pair(-1, -1));
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

std::string Response::generateFileListPage(const std::string &uri,
																					 const std ::string &root,
																					 const std::vector<std::string> &files) const
{
	std::stringstream html;
	std::size_t totalFiles = files.size();

	html << "<html>\n";
	html << "<head><title>Index of " << uri << "</title></head>\n";
	html << "<body>\n";
	html << "<h1>Index of " << uri << "</h1><hr><pre><a href=\"../\">../</a>\n";

	for (std::size_t i = 0; i < totalFiles; ++i)
	{
		// skip if dot file
		if (files[i][0] == '.')
			continue;

		const std::string path = root + files[i];
		struct stat st;

		if (stat(path.c_str(), &st) == -1)
		{
			Logger::error() << files[i] << ": " << std::strerror(errno) << std::endl;
			continue;
		}

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
	targetBody.buffer.clear();
	targetBody.file.close();
	targetBody.file.clear();
}

size_t Response::getFileSize()
{
	std::ifstream &file = this->body.file;
	size_t size = 0;

	if (file)
	{
		file.seekg(0, file.end);
		size = file.tellg();
		file.seekg(0, file.beg);
	}
	return (size);
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
	else
		Logger::error() << "Response::readDirectory: opendir: " << std::strerror(errno) << std::endl;
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

bool Response::isDirectory(const std::string &path)
{
	DIR *dir;

	dir = opendir(path.c_str());
	if (dir)
	{
		closedir(dir);
		return (true);
	}
	else
		return (false);
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
