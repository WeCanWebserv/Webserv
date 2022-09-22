#ifndef SERVER_MANAGER_HPP
#define SERVER_MANAGER_HPP

#include <sys/epoll.h>

#include <map>
#include <set>

#include "Config.hpp"
#include "Connection.hpp"
#include "response/Response.hpp"

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 32768
#endif // BUFFER_SIZE

#define DEFAULT_PATH "../config/default.conf"
#define MAX_EVENTS 10
#define MAX_BACKLOGS 10

class ServerManager
{
public:
	static const char *gDefaultPath;
	static const int gMaxEvents = MAX_EVENTS;
	static const int gBackLog = 1;

private:
	typedef std::map<int, const ServerConfig> server_container_type;
	typedef std::map<int, Connection> connection_container_type;
	typedef std::map<int, int> extra_fd_container_type;

	char buffer[BUFFER_SIZE];
	extra_fd_container_type extraFds;
	connection_container_type connections;
	server_container_type servers;
	std::set<int> fdInUse;
	int epollFd;

public:
	ServerManager(const char *path = DEFAULT_PATH);
	~ServerManager();

	void loop();
	void clear();

protected:
	// used in connect()
	int addEvent(int fd, int option);
	// used in disconnect()
	int deleteEvent(int fd);

	// used in loop()
	int modifyEvent(int fd, struct epoll_event &event, int option);
	void connect(int serverFd);
	void disconnect(int clientFd);
	int receive(int fd);
	int send(int fd, Response &response);

	void registerResposneEvent(int eventFd, Response &res, std::pair<int, int> newEvent);

private:
	ServerManager();
	ServerManager &operator=(const ServerManager &);
	ServerManager(const ServerManager &);
};

#endif // SEVER_MANAGER_HPP
