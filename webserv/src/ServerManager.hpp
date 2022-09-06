#ifndef SERVER_MANAGER_HPP
#define SERVER_MANAGER_HPP

#include <sys/epoll.h>

#include <map>

#include "ConfigInfo.hpp"
#include "Connection.hpp"
#include "response/Response.hpp"

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 32768
#endif // BUFFER_SIZE

#define MAX_EVENTS 1024
#define MAX_BACKLOGS 42

class ServerManager
{
public:
	static const int gMaxEvents = MAX_EVENTS;
	static const int gBackLog = MAX_BACKLOGS;

private:
	typedef std::map<int, ConfigInfo> server_container_type;
	typedef std::map<int, Connection> connection_container_type;
	typedef std::map<int, int> extra_fd_container_type;

	char buffer[BUFFER_SIZE];
	int epollFd;
	server_container_type servers;
	connection_container_type connections;
	extra_fd_container_type extraFds;

public:
	ServerManager(const char *path);
	~ServerManager();

	void loop();

protected:
	// used in connect()
	int addEvent(int clientFd, int option);
	// used in disconnect()
	int deleteEvent(int clientFd);

	// used in loop()
	int modifyEvent(int clientFd, struct epoll_event &event, int option);
	void connect(int serverFd);
	void disconnect(int clientFd);
	int receive(int cliendfd);
	int send(int clinetfd, Response &response);

private:
	ServerManager();
	ServerManager &operator=(const ServerManager &);
	ServerManager(const ServerManager &);
};

#endif // SEVER_MANAGER_HPP
