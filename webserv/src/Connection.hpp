#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "request/request_manager.hpp"
#include "response/Response.hpp"

class Connection
{
private:
	int serverFd;
	RequestManager requestManager;
	Response response;

public:
	Connection() {}
	Connection(int serverFd) : serverFd(serverFd) {}

	void setServerFd(int serverFd)
	{
		this->serverFd = serverFd;
	}

	RequestManager &getRequestManager()
	{
		return this->requestManager;
	}
	Response &getResponse()
	{
		return this->response;
	}

	void clear();
};

#endif // CONNECTION_HPP
