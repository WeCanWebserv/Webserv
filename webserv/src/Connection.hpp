#ifndef CONNECTION_HPP
#define CONNECTION_HPP

class Request
{
public:
	Request() {}
	~Request() {}
	bool ready()
	{
		return false;
	}
};

class Response
{
public:
	Response() {}
	~Response() {}
	bool ready()
	{
		return false;
	}
	bool sentAll()
	{
		return false;
	}
};

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
};

#endif // CONNECTION_HPP