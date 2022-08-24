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

#include "ConfigInfo.hpp"
#include "ConfigParser.hpp"

ServerManager::ServerManager(const char *path)
{
	if (!path)
		path = "./config/default.conf";
	try
	{
		epollFd = epoll_create(this->gMaxEvents);
		if (epollFd == -1)
			throw std::runtime_error("epoll_create");

		ConfigParser parser(path);
		std::vector<ConfigInfo> infos;
		for (;;)
		{
			if (!parser.remain())
				break;
			ConfigInfo info = parser.getInfo();
			infos.push_back(info);
		}
		for (std::vector<ConfigInfo>::iterator infoIter = infos.begin(); infoIter != infos.end();
				 infoIter++)
		{
			int host = infoIter->getHost();
			struct protoent *proto = getprotobyname("tcp");
			if (!proto)
				throw std::runtime_error("getprotobyname");

			const std::vector<int> &ports = infoIter->getPorts();
			struct sockaddr_in sockAddr;
			sockAddr.sin_family = AF_INET;
			for (std::vector<int>::iterator portIter; portIter != ports.end(); portIter++)
			{
				int socketFd = socket(PF_INET, SOCK_STREAM, proto->p_proto);
				if (socketFd == -1)
					throw std::runtime_error("socket");

				int port = *portIter;
				sockAddr.sin_port = htons(port);
				sockAddr.sin_addr.s_addr = htonl(host);
				if (bind(socketFd, (const struct sockaddr *)&sockAddr, sizeof(sockAddr)) == -1)
					throw std::runtime_error("bind");

				const std::pair<server_container_type::iterator, bool> &result =
						servers.insert(std::make_pair(socketFd, *infoIter));
				if (!result.second)
					throw std::runtime_error("servers.insert");
			}
		}
		for (std::map<int, ConfigInfo>::iterator servIter; servIter != servers.end(); servIter++)
		{
			int serverFd = servIter->first;
			if (listen(serverFd, this->gBackLog) == -1)
				throw std::runtime_error("listen");
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << '\n';
	}
}

ServerManager::~ServerManager()
{
	for (server_container_type::iterator servIter = this->servers.begin();
			 servIter != this->servers.end(); servIter++)
	{
		close(servIter->first);
	}
	for (connection_container_type::iterator connIter = this->connections.begin();
			 connIter != this->connection.end(); connIter++)
	{
		close(connIter->first);
	}
	close(this->epollFd);
}

void ServerManager::loop()
{
	struct epoll_event events[this->gMaxEvents];
	struct epoll_event currentEvent;

	try
	{
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
					{
						this->disconnect(eventFd);
					}
					else if (occurredEvent & EPOLLIN)
					{
						if (this->receive(eventFd) == -1)
							throw std::runtime_error("recv");

						Request &request = connection.getRequest();
						if (!request.ready())
							continue;
						// TODO: Request Buffer Handling @jungwkim
						// ...
						// example:
						// Response &response = connection.getResponse();
						// response.populate(request);
						currentEvent.events |= EPOLLOUT;
						epoll_ctl(this->epollFd, EPOLL_CTL_MOD, eventFd, &currentEvent)
					}
					else
					{
						Response &response = connection.getResponse();
						if (!response.ready())
						{
							// TODO: Response FILE I/O, PIPE I/O Handling @seushin
						}
						else
						{
							if (this->send(eventFd) == -1)
								throw std::runtime_error("send");

							if (!response.sentAll())
								continue;
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
	catch (std::exception &e)
	{
		std::cerr << e.what() << std::endl;
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
	int clientFd =
			accept(serverFd, (struct sockaddr *)&clientAddr,
						 (socklen_t *)&client_len) if (clientFd == -1) throw std::runtime_error("accept");

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
	if (nbytes == 0)
		return -1;
	else
	{
		// ...
		return 0;
	}
}
