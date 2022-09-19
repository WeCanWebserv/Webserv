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
	Connection();
	Connection(int serverFd);

	void setServerFd(int serverFd);
	int getServerFd() const;
	RequestManager &getRequestManager();
	Response &getResponse();

	void clear();
};

#endif // CONNECTION_HPP
