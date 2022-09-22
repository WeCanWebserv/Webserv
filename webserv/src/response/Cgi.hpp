#ifndef CGI_HPP
#define CGI_HPP

#include "../Config.hpp"
#include "../request/request.hpp"

#include <sstream>
#include <string>
#include <unistd.h>

class Response;

class Cgi
{
public:
	bool isCgi;
	pid_t pid;
	int fd[2];

public:
	Cgi();
	Cgi(const Cgi &other);
	~Cgi();

	operator bool() const;

	bool fail();

	void clear();

	int run(Request &req, const ServerConfig &config, const LocationConfig &location, int clientFd);
	char **generateMetaVariables(Request &req,
															 const ServerConfig &config,
															 const LocationConfig &location,
															 int clientFd);

	int parseCgiResponse(Response &res);
	int parseStatusHeader(std::stringstream &ss);
	size_t parseContentLength(std::stringstream &ss);
};

#endif // !CGI_HPP
