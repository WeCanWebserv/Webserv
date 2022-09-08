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

	template<class Request>
	int run(Request &req, int serverFd, int clientFd, const std::string &root);
	template<class Request>
	char **generateMetaVariables(Request &req, int serverFd, int clientFd, const std::string &root);
};

#endif // !CGI_HPP
