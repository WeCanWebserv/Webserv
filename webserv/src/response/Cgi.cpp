#include "Cgi.hpp"
#include "../request/request.hpp"

#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <map>
#include <utility>

namespace ft
{
char *strdup(const std::string &str)
{
	char *res;

	res = new char[str.size() + 1];
	res[0] = '\0';
	std::strcpy(res, str.c_str());
	return (res);
}

template<class T>
std::string toString(T value)
{
	std::stringstream ss;

	ss << value;
	return (ss.str());
}

int toUnderscore(int c)
{
	if (c == '-')
		return ('_');
	return (c);
}

template<class UnaryOP>
std::string transform(const std::string &str, UnaryOP op)
{
	std::string up = str;

	std::transform(up.begin(), up.end(), up.begin(), op);
	return (up);
}

} // namespace ft

Cgi::Cgi() : isCgi(false), pid(-1) {}

Cgi::Cgi(const Cgi &other) : isCgi(other.isCgi), pid(other.pid)
{
	std::memmove(this->fd, other.fd, sizeof(this->fd));
}

Cgi::~Cgi() {}

/**
 * run(Request, ServerConfig, location, clientFd)
 */
template<class Request, class Config, class Location>
int Cgi::run(Request &req, Config &config, Location &location, int clientFd)
{
	int reqPipe[2];
	int resPipe[2];

	/**
	 *    server     |      cgi
	 *
	 *          reqPipe
	 *      ---------------->
	 *
	 *          resPipe
	 *      <----------------
	 *
	 */
	if (pipe(reqPipe) == -1)
		return (1);
	if (pipe(resPipe) == -1)
	{
		close(reqPipe[0]);
		close(reqPipe[1]);
		return (1);
	}

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

		char **cmd; // = { "location.cgi", "uri.scriptName" NULL }
		char **env = generateMetaVariables(req, config, location, clientFd);
		int err = execve(cmd[0], cmd, env);
		exit(err);
	}
	else
	{
		close(reqPipe[0]);
		close(resPipe[1]);

		fd[0] = resPipe[0];
		fd[1] = reqPipe[1];
	}
	return (0);
}

template<class Request, class Config, class Location>
char **Cgi::generateMetaVariables(Request &req, Config &config, Location &location, int clientFd)
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
	env["SERVER_NAME"] = config.listOfServerNames;
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
	// TODO: uri parsing
	env["REQUEST_METHOD"] = req.method;
	env["REQUEST_URI"] = req.uri;
	env["DOCUMENT_URI"]; // uri에서 query 전까지 config의 location을 제외해야 할까
	env["DOCUMENT_ROOT"] = location.root; // config root
	env["SCRIPT_NAME"];                   // uri에서 script 파일까지
	env["SCRIPT_FILENAME"];               // DOCUMENT_ROOT + SCRIPT_NAME

	// optional uri data
	env["PATH_INFO"];       // uri에서 SCRIPT_NAME 뒤에 오는 경로
	env["PATH_TRANSLATED"]; // DOCUMENT_ROOT  + PATH_INFO
	env["QUERY_STRING"];

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

	result = new char *[envSize];
	for (env_type::iterator it = env.begin(), ite = env.end(); it != ite; ++it, ++i)
	{
		result[i] = ft::strdup(it->first + "=" + it->second);
	}
	return (result);
}
