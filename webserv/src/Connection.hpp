#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <time.h>

#include "request/request_manager.hpp"
#include "response/Response.hpp"

#define MAX_TRANSACTION 20
#define KEEP_ALIVE_TIME 20

class Connection
{
public:
	static const int maxTransaction = MAX_TRANSACTION;
	static const int keepAliveTime = KEEP_ALIVE_TIME;

private:
	RequestManager requestManager;
	int serverFd;
	Response response;
	time_t lastAccessTime;
	int transactionCount;

public:
	Connection();
	Connection(int serverFd, size_t maxBodySize);

	void setLastAcceesTime(time_t time);
	void setServerFd(int serverFd);
	void setMaxBodySize(size_t maxBodySize);
	time_t getLastAccessTime() const;
	int getServerFd() const;
	int getTransactionCount() const;
	RequestManager &getRequestManager();
	Response &getResponse();
	bool checkTimeOut();
	void increaseTransactionCount();
	void clear();
};

#endif // CONNECTION_HPP
