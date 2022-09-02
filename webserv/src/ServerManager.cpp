#include "ServerManager.hpp"

#include <netdb.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <exception>
#include <iostream>

#include <map>
#include <utility>
#include <vector>

#include "Config.hpp"
#include "ConfigParser.hpp"

#include "Connection.hpp"

#include <iostream>

ServerManager::ServerManager(const char *path)
{
	const ConfigParser parser(path);
	const Config &config = parser.getConfig();
	const std::vector<ServerConfig> &serverConfigs = config.serverConfigs;

	struct sockaddr_in sockAddr;
	sockAddr.sin_family = AF_INET;
	for (size_t idx = 0; idx < serverConfigs.size(); idx++)
	{
		int socketFd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
		if (socketFd == -1)
			throw std::runtime_error("socket");
		else
			this->fdInUse.insert(socketFd);

		ServerConfig serverConfig = serverConfigs[idx];
		sockAddr.sin_addr.s_addr = serverConfig.listennedHost;
		sockAddr.sin_port = serverConfig.listennedPort;
		if (bind(socketFd, (const struct sockaddr *)&sockAddr, sizeof(sockAddr)) == -1)
			throw std::runtime_error("bind");

		if (!servers.insert(std::make_pair(socketFd, serverConfig)).second)
			throw std::runtime_error("map.insert");
	}

	for (server_container_type::iterator iter = servers.begin(); iter != servers.end(); iter++)
	{
		int serverFd = iter->first;
		if (listen(serverFd, this->gBackLog) == -1)
			throw std::runtime_error("listen");
	}

	this->epollFd = epoll_create(this->gMaxEvents);
	if (this->epollFd == -1)
		throw std::runtime_error("epoll_create");
}

ServerManager::~ServerManager()
{
	this->clear();
	close(this->epollFd);
}

void ServerManager::clear()
{
	for (std::set<int>::iterator iter; iter != this->fdInUse.end(); iter++)
	{
		int fd = *iter;
		close(fd);
	}
}

void ServerManager::loop()
{
	struct epoll_event events[this->gMaxEvents];
	struct epoll_event currentEvent;

	for (;;)
	{
		int nfds = epoll_wait(this->epollFd, events, this->gMaxEvents - 1, 0);
		if (nfds == -1)
			throw std::runtime_error("epoll_wait");
		for (int idx = 0; idx < nfds; idx++)
		{
			int eventFd = events[idx].data.fd;
			server_container_type::iterator found = servers.find(eventFd);
			if (found != servers.end())
				this->connect(found->first);
			else
			{
				connection_container_type::iterator found = connections.find(eventFd);
				if (found == connections.end())
					throw std::runtime_error("connection not found");

				Connection &connection = found->second;
				int occurredEvent = events[idx].events;
				if (occurredEvent & EPOLLERR || occurredEvent & EPOLLHUP)
					this->disconnect(eventFd);
				else if (occurredEvent & EPOLLIN)
				{
					if (this->receive(eventFd) == -1)
						this->disconnect(eventFd);
					else
					{
						Request &request = connection.getRequest();
						if (!request.ready()) // read
						{
							// TODO: Request Buffer Handling @jungwkim
							// ...
						}
						if (request.ready()) // open
						{
							Response &response = connection.getResponse();
							// response.populate(request);
							//
						}
						// if (response.ready())
						// {
						// 	// this->modifyEvent(eventFd, currentEvent, EPOLLIN | EPOLLOUT);
						// }
					}
				}
				else
				{
					Response &response = connection.getResponse();
					if (!response.ready())
					{
						// connecion
						// TODO: Response FILE I/O, PIPE I/O Handling @seushin

						continue;
					}
					if (this->send(eventFd) == -1)
						this->disconnect(eventFd);
					// else if (!response.done())
					// continue;
					else
					{
						// if (response.keepAlive())
						// {
						//    if (modifyEvent(eventFd, currentEvent, EPOLLIN) == -1)
						//       throw std::runtime_error("modifyEvent");
						//    connection.clear(); // use request.clear(), response.claer())
						// }
						// else
						//  this->disconnect(eventFd);
						//
					}
				}
			}
		}
		// TODO:
		// for (connection_container_type::iterator connIter = connections.begin(); contIter != connections.end(); connIter++)
		// {
		//   Connection& connection = connIter.second;
		//   if (connection.checkTimeOut())
		//     this->disconnect(connIter.first);
		// }
	}
}

// used in connect()
int ServerManager::addEvent(int clientFd)
{
	struct epoll_event event;
	event.events |= EPOLLIN;
	return epoll_ctl(this->epollFd, EPOLL_CTL_ADD, clientFd, &event);
}

// used in disconnect()
int ServerManager::deleteEvent(int clientFd)
{
	return epoll_ctl(this->epollFd, EPOLL_CTL_DEL, clientFd, 0);
}

// used in loop()
int ServerManager::modifyEvent(int clientFd, struct epoll_event &event, int option)
{
	event.events = option;
	return epoll_ctl(this->epollFd, EPOLL_CTL_MOD, clientFd, &event);
}

void ServerManager::connect(int serverFd)
{
	struct sockaddr_in clientAddr;
	int clientLength = sizeof(clientAddr);
	int clientFd = accept(serverFd, (struct sockaddr *)&clientAddr, (socklen_t *)&clientLength);
	if (clientFd == -1)
		throw std::runtime_error("accept");

	Connection newConnection(serverFd);
	connections.insert(std::make_pair(clientFd, newConnection));
	if (this->addEvent(clientFd) == -1)
		throw std::runtime_error("addEvent");
}

void ServerManager::disconnect(int clientFd)
{
	this->deleteEvent(clientFd);
	connections.erase(clientFd);
	close(clientFd);
}

int ServerManager::receive(int fd)
{
	char *buffer = this->buffer;
	int nbytes = recv(fd, buffer, BUFFER_SIZE, 0);
	if (nbytes == 0)
		return -1;
	else
	{
		// example:
		// Request& request = connections[fd].getRequest();
		// request.append(buffer);
	}
	return 0;
}

int ServerManager::send(int fd)
{
	Response &response = connections[fd].getResponse();
	char *buffer = this->buffer; // TODO: response.?()
	int nbytes = ::send(fd, buffer, BUFFER_SIZE, 0);
	if (nbytes == -1)
		return -1;
	else
	{
		// ...
		return 0;
	}
}