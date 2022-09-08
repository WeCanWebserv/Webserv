#ifndef CGI_HPP
#define CGI_HPP

#include "Response.hpp"

#include <string>
#include <unistd.h>

class Cgi
{
private:
	bool isCgi;
	pid_t pid;
	int fd[2];

public:
	Cgi();
	Cgi(const Cgi &other);
	~Cgi();

	template<class Request, class Config, class Location>
	int run(Request &req, Config &config, Location &location, int clientFd);
	template<class Request, class Config, class Location>
	char **generateMetaVariables(Request &req, Config &config, Location &location, int clientFd);
};

#endif // !CGI_HPP
