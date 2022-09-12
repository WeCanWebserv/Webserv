#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "response/Response.hpp"

// class Request
// {
// public:
// 	Request() {}
// 	~Request() {}
// 	bool ready()
// 	{
// 		return false;
// 	}
// };

class Connection
{
private:
	int serverFd;
	Request request;
	Response response;

public:
	Connection() {}
	Connection(int serverFd) : serverFd(serverFd) {}

	Request &getRequest()
	{
		return this->request;
	}
	Response &getResponse()
	{
		return this->response;
	}

	void clear() {}
};

#endif // CONNECTION_HPP
