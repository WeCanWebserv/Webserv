#include "Cgi.hpp"

#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

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
template<class Request>
int Cgi::run(Request &req, int serverFd, int clientFd, const std::string &root)
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
		char **env = generateMetaVariables(req, serverFd, clientFd, root);
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

template<class Request>
char **Cgi::generateMetaVariables(Request &req, int serverFd, int clientFd, const std::string &root)
{
	char **result;
	std::map<std::string, std::string> env;
	std::map<std::string, std::string>::iterator it, ite;

	struct sockaddr_in addr;
	socklen_t addrLen = sizeof(addr);

	getsockname(clientFd, reinterpret_cast<struct sockaddr *>(&addr), &addrLen);
	env["REMOTE_ADDR"] = ft::toString(ntohl(addr.sin_addr.s_addr));
	env["REMOTE_PORT"] = ft::toString(ntohs(addr.sin_port));

	// FIX: serverConfig에서 받아오기
	getsockname(serverFd, reinterpret_cast<struct sockaddr *>(&addr), &addrLen);
	env["SERVER_ADDR"] = ft::toString(ntohl(addr.sin_addr.s_addr));
	env["SERVER_PORT"] = ft::toString(ntohs(addr.sin_port));
	// env["SERVER_NAME"] = config.serverName;

	env["GATEWAY_INTERFACE"] = "CGI/1.1";
	env["SERVER_PROTOCOL"] = "HTTP/1.1";
	env["SERVER_SOFTWARE"] = "webserv";

	// parse uri
	env["REQUEST_METHOD"] = req.method;
	env["REQUEST_URI"] = req.uri;
	env["DOCUMENT_URI"]; // uri에서 query 전까지 config의 location을 제외해야 할까
	env["DOCUMENT_ROOT"] = root; // config root
	env["SCRIPT_NAME"];          // uri에서 script 파일까지
	env["SCRIPT_FILENAME"];      // DOCUMENT_ROOT + SCRIPT_NAME

	// php
	env["REDIRECT_STATUS"] = "200";

	// optional
	env["PATH_INFO"];       // uri에서 SCRIPT_NAME 뒤에 오는 경로
	env["PATH_TRANSLATED"]; // DOCUMENT_ROOT  + PATH_INFO
	env["QUERY_STRING"];
	if (req.body)
	{
		env["CONTENT_LENGTH"];
		env["CONTENT_TYPE"];
	}

	// TODO: request header "HTTP_" prefix와 함께 환경변수로 전달하기

	const size_t envSize = env.size();
	int i;

	result = new char *[envSize];
	for (i = 0, it = env.begin(), ite = env.end(); it != ite; ++i, ++it)
	{
		result[i] = ft::strdup((*it).first + "=" + (*it).second);
	}
	return (result);
}
