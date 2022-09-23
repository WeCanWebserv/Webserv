#include "Connection.hpp"

Connection::Connection() : requestManager(0) {}

Connection::Connection(int serverFd, size_t maxBodySize) : serverFd(serverFd), requestManager(maxBodySize) {}

void Connection::setServerFd(int serverFd)
{
	this->serverFd = serverFd;
}

void Connection::setMaxBodySize(size_t maxBodySize)
{
	this->requestManager.setMaxBodySize(maxBodySize);
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
