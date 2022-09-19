#include "Connection.hpp"

Connection::Connection() {}

Connection::Connection(int serverFd) : serverFd(serverFd) {}

void Connection::setServerFd(int serverFd)
{
	this->serverFd = serverFd;
}

RequestManager &Connection::getRequestManager()
{
	return this->requestManager;
}
Response &Connection::getResponse()
{
	return this->response;
}

void Connection::clear()
{
	this->getResponse().clear();
}

int Connection::getServerFd() const
{
	return (this->serverFd);
}
