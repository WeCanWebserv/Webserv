#include "Connection.hpp"

#include <time.h>

Connection::Connection() : requestManager(0), lastAccessTime(time(NULL)), transactionCount(0) {}

Connection::Connection(int serverFd, size_t maxBodySize)
		: requestManager(maxBodySize), serverFd(serverFd), lastAccessTime(time(NULL)),
			transactionCount(0)
{}

void Connection::setLastAcceesTime(time_t time)
{
	this->lastAccessTime = time;
}

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

int Connection::getTransactionCount() const
{
	return (this->transactionCount);
}

time_t Connection::getLastAccessTime() const
{
	return this->lastAccessTime;
}

bool Connection::checkTimeOut()
{
	time_t currentTime = time(NULL);
	bool isTimeOut = currentTime - this->lastAccessTime > this->keepAliveTime ? true : false;
	return isTimeOut;
}

void Connection::increaseTransactionCount()
{
	this->transactionCount++;
}