#include "Cgi.hpp"
#include "../Logger.hpp"
#include "../libft.hpp"
#include "../request/request.hpp"
#include "Response.hpp"
#include "UriParser.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <map>
#include <sstream>
#include <string>
#include <utility>

Cgi::Cgi() : isCgi(false), pid(-1) {}

Cgi::Cgi(const Cgi &other) : isCgi(other.isCgi), pid(other.pid)
{
	std::memmove(this->fd, other.fd, sizeof(this->fd));
}

Cgi::~Cgi() {}

Cgi::operator bool() const
{
	return (this->isCgi);
}

bool Cgi::fail()
{
	int err;
	int status;

	err = waitpid(this->pid, &status, WNOHANG);
	if (err == -1)
	{
		Logger::error() << "Cgi: process " << this->pid << " waitpid: " << std::strerror(errno)
										<< std::endl;
		return (true);
	}
	else if (err == 0)
		return (false); // cgi in progress

	Logger::info() << "Cgi: process " << this->pid << " exit code: " << WEXITSTATUS(status)
								 << " with signal " << WTERMSIG(status) << std::endl;
	return (status != 0);
}

int Cgi::exitCode() const
{
	int status;

	waitpid(this->pid, &status, WNOHANG);

	Logger::info() << "Cgi: process " << this->pid << " exit code: " << WEXITSTATUS(status)
								 << " with signal " << WTERMSIG(status) << std::endl;
	return (status);
}

void Cgi::clear()
{
	this->pid = -1;
	this->isCgi = false;
}

int Cgi::run(Request &req, const ServerConfig &config, const LocationConfig &location, int clientFd)
{
	int reqPipe[2];
	int resPipe[2];

	/**
	 *    server     |      cgi
	 *
	 *          reqPipe
	 *      ---------------->
	 *          resPipe
	 *      <----------------
	 */
	this->isCgi = true;
	if (pipe(reqPipe) == -1)
		return (1);
	if (pipe(resPipe) == -1)
	{
		close(reqPipe[0]);
		close(reqPipe[1]);
		return (1);
	}

	fcntl(reqPipe[0], F_SETFL, O_NONBLOCK);
	fcntl(reqPipe[1], F_SETFL, O_NONBLOCK);
	fcntl(resPipe[0], F_SETFL, O_NONBLOCK);
	fcntl(resPipe[1], F_SETFL, O_NONBLOCK);

	this->pid = fork();
	if (this->pid == -1)
	{
		close(reqPipe[0]);
		close(reqPipe[1]);

		close(resPipe[0]);
		close(resPipe[1]);
		return (1);
	}
	else if (this->pid == 0)
	{
		dup2(reqPipe[0], STDIN_FILENO);
		close(reqPipe[1]);

		close(resPipe[0]);
		dup2(resPipe[1], STDOUT_FILENO);

		UriParser uriParser(req.getStartline().uri);
		char *cmd[] = {ft::strdup(location.tableOfCgiBins.at(uriParser.getExtension())),
									 ft::strdup(location.root + uriParser.getFile()), NULL};
		char **env = generateMetaVariables(req, config, location, clientFd);
		execve(cmd[0], cmd, env);
		exit(errno);
	}
	else
	{
		Logger::info() << "Cgi: running cgi process... pid: " << this->pid << std::endl;
		close(reqPipe[0]);
		close(resPipe[1]);

		this->fd[0] = resPipe[0];
		this->fd[1] = reqPipe[1];
	}
	return (0);
}

char **Cgi::generateMetaVariables(Request &req,
																	const ServerConfig &config,
																	const LocationConfig &location,
																	int clientFd)
{
	typedef std::map<std::string, std::string> env_type;

	char **result;
	env_type env;

	env["GATEWAY_INTERFACE"] = "CGI/1.1";

	/**
	 * for php-cgi
	 */
	env["REDIRECT_STATUS"] = "200";

	/**
	 * server info
	 */
	env["SERVER_ADDR"] = config.listennedHost;
	env["SERVER_PORT"] = config.listennedPort;
	env["SERVER_NAME"] = config.listOfServerNames.front();
	env["SERVER_PROTOCOL"] = "HTTP/1.1";
	env["SERVER_SOFTWARE"] = "webserv";

	/**
	 * client info
	 */
	struct sockaddr_in addr;
	socklen_t addrLen = sizeof(addr);

	getsockname(clientFd, reinterpret_cast<struct sockaddr *>(&addr), &addrLen);
	env["REMOTE_ADDR"] = ft::toString(ntohl(addr.sin_addr.s_addr));
	env["REMOTE_PORT"] = ft::toString(ntohs(addr.sin_port));

	/**
	 * uri
	 */
	UriParser uriParser(req.getStartline().uri);

	env["REQUEST_METHOD"] = req.getStartline().method;
	env["REQUEST_URI"] = req.getStartline().uri;
	env["DOCUMENT_URI"] = uriParser.getPath();
	env["DOCUMENT_ROOT"] = location.root;
	env["SCRIPT_NAME"] = uriParser.getFile();
	env["SCRIPT_FILENAME"] = env["DOCUMENT_ROOT"] + env["SCRIPT_NAME"];

	std::string pathInfo = uriParser.getPathInfo();

	if (pathInfo.size())
	{
		env["PATH_INFO"] = pathInfo; // uri에서 SCRIPT_NAME 뒤에 오는 경로
		env["PATH_TRANSLATED"] = env["DOCUMENT_ROOT"] + env["PATH_INFO"];
	}

	std::string query = uriParser.getQuery();

	if (query.size())
		env["QUERY_STRING"] = query;

	/**
	 * Request Header
	 */
	const Header::HeaderMap &headers = req.getHeader().getFields();

	for (Header::HeaderMap::const_iterator it = headers.begin(), ite = headers.end(); it != ite; ++it)
	{
		std::string fieldName = it->first;

		fieldName = ft::transform(fieldName, ::toupper);
		fieldName = ft::transform(fieldName, ft::toUnderscore);
		env["HTTP_" + fieldName] = ""; // FIX: std::vector<FieldValue> => toString;
	}

	/**
	 * Reqeust Body
	 */
	const Body &body = req.getBody();

	if (body.payload.size())
	{
		env["CONTENT_LENGTH"] = ft::toString(body.payload.size());
		env["CONTENT_TYPE"] = env["HTTP_CONTENT_TYPE"];
	}

	/**
	 * convert to char** from std::map
	 */
	const size_t envSize = env.size();
	int i = 0;

	result = new char *[envSize + 1];
	for (env_type::iterator it = env.begin(), ite = env.end(); it != ite; ++it, ++i)
	{
		result[i] = ft::strdup(it->first + "=" + it->second);
	}
	result[envSize] = NULL;
	return (result);
}

int Cgi::parseCgiResponse(Response &res)
{
	int statusCode = parseStatusHeader(res.body.buffer);
	size_t contentLength = parseContentLength(res.body.buffer);

	res.setStatusCode(statusCode);
	if (contentLength != 0)
		res.setHeader("Content-Length", ft::toString(contentLength));
	return (0);
}

int Cgi::parseStatusHeader(std::stringstream &ss)
{
	/**
	 * 첫 라인이 "status: "라면 버퍼에서 status code 파싱
	 * 아니라면 200 OK
	 */
	const std::string statusField = "status: ";
	std::string firstLine;
	int statusCode = 200;

	if (std::getline(ss, firstLine))
	{
		firstLine = ft::transform(firstLine, ::tolower);

		if (firstLine.compare(0, statusField.size(), statusField) == 0)
		{
			std::string status = firstLine.substr(statusField.size());
			statusCode = std::atoi(status.c_str());
		}
		else
		{
			ss.seekg(0, ss.beg);
		}
	}

	/**
	 * reset flag & position
	 */
	ss.clear();

	return (statusCode);
}

size_t Cgi::parseContentLength(std::stringstream &ss)
{
	const std::string endOfHeader = "\r\n\r\n";
	std::string buffer = ss.str();
	std::string::size_type pos;

	pos = buffer.find(endOfHeader);
	if (pos == std::string::npos)
		return (0);

	return (buffer.size() - (pos + endOfHeader.size()));
}
