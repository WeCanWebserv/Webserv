#include "ServerManager.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
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
#include "response/Response.hpp"

#include <iostream>

ServerManager::ServerManager(const char *path)
{
	this->epollFd = epoll_create(this->gMaxEvents);
	if (this->epollFd == -1)
		throw std::runtime_error("epoll_create");

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
		{
			this->fdInUse.insert(socketFd);
			int optVal = 1;
			setsockopt(socketFd, SOL_SOCKET, SO_REUSEPORT, (void *)&optVal, (socklen_t)sizeof(optVal));
		}

		const ServerConfig &serverConfig = serverConfigs[idx];
		sockAddr.sin_addr.s_addr = inet_addr(serverConfig.listennedHost.c_str());
		sockAddr.sin_port = htons(std::atoi(serverConfig.listennedPort.c_str()));
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
		this->addEvent(serverFd, EPOLLIN);
	}
}

ServerManager::~ServerManager()
{
	this->clear();
	close(this->epollFd);
}

void ServerManager::clear()
{
	for (std::set<int>::iterator iter = this->fdInUse.begin(); iter != this->fdInUse.end(); iter++)
	{
		int fd = *iter;
		close(fd);
	}
}

void ServerManager::loop()
{
	struct epoll_event events[MAX_EVENTS];
	struct epoll_event currentEvent;

	for (;;)
	{
		int nfds = epoll_wait(this->epollFd, events, MAX_EVENTS, -1);
		if (nfds == -1)
			throw std::runtime_error("epoll_wait");

		for (int idx = 0; idx < nfds; idx++)
		{
			int eventFd = events[idx].data.fd;
			int occurredEvent = events[idx].events;

			/**
			 * 서버 소켓의 커넥션 요청 이벤트
			 */
			server_container_type::iterator foundServer = servers.find(eventFd);
			if (foundServer != servers.end() && occurredEvent & EPOLLIN)
			{
				this->connect(eventFd);
				continue;
			}

			/**
			 * 클라이언트 소켓 이벤트
			 * HTTP Request 혹은 HTTP Response
			 */
			connection_container_type::iterator foundConn = connections.find(eventFd);
			if (foundConn != connections.end())
			{
				Connection &connection = foundConn->second;
				Request &request = connection.getRequest();
				Response &response = connection.getResponse();

				if (occurredEvent & EPOLLERR || occurredEvent & EPOLLHUP)
					this->disconnect(eventFd);
				else if (occurredEvent & EPOLLIN)
				{
					std::pair<int, int> newEvent(-1, 0);

					try
					{
						if (this->receive(eventFd) == -1)
						{
							this->disconnect(eventFd);
							continue;
						}
						// TODO: Request Buffer Handling @jungwkim
						// ...

						// if (request.ready())
						// {
						// 	newEvent = response.process(request, servers[eventFd], eventFd);
						// }
					}
					catch (int errorCode)
					{
						newEvent = response.process(errorCode, servers[eventFd]);
					}
					catch (...)
					{
						this->disconnect(eventFd);
						continue;
					}

					/**
					 * Response 생성을 위한 파일/cgi 이벤트 등록
					 */
					if (newEvent.first != -1)
					{
						if (this->addEvent(newEvent.first, newEvent.second) == -1)
							this->disconnect(eventFd);
					}
					/**
					 * Response가 버퍼를 직접 채운 경우(directory listing, default error page)
					 * 추가 이벤트 없이 즉시 응답을 보낸다
					 */
					else if (response.ready())
					{
						if (this->modifyEvent(eventFd, currentEvent, EPOLLIN | EPOLLOUT) == -1)
							this->disconnect(eventFd);
					}
				}
				else if (occurredEvent & EPOLLOUT)
				{
					if (this->send(eventFd, response) <= 0)
					{
						this->disconnect(eventFd);
						continue;
					}

					if (response.done())
					{
						if (response.close())
							this->disconnect(eventFd);
						else
						{
							if (this->modifyEvent(eventFd, currentEvent, EPOLLIN) != -1)
								connection.clear(); // use request.clear(), response.claer())
							else
								this->disconnect(eventFd);
						}
					}
				}
			}
			else
			{
				/**
				 * HTTP Response에 해당하는 파일/cgi 이벤트
				 */
				extra_fd_container_type::iterator foundFd = extraFds.find(eventFd);
				connection_container_type::iterator foundConn = connections.find(foundFd->second);

				if (foundFd == extraFds.end() || foundConn == connections.end())
				{
					this->deleteEvent(eventFd);
					close(eventFd);
					continue;
				}

				Connection &connection = foundConn->second;
				Response &response = connection.getResponse();

				if (occurredEvent & EPOLLIN)
				{
					int n;

					n = response.readBody();
					// FIX: read가 실패했을 때 연결 해제? 에러 응답?
					if (n <= 0)
					{
						this->deleteEvent(eventFd);
						close(eventFd);
						this->modifyEvent(foundFd->second, currentEvent, EPOLLIN | EPOLLOUT);
					}
				}
				else if (occurredEvent & EPOLLOUT)
				{
					int pipe = response.writeBody();

					if (pipe > 0)
					{
						this->deleteEvent(eventFd);
						close(eventFd);
						this->addEvent(pipe, EPOLLIN);
					}
					else if (pipe == -1)
					{
						// write error
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
int ServerManager::addEvent(int fd, int option)
{
	struct epoll_event event;
	event.events = option;
	event.data.fd = fd;
	return epoll_ctl(this->epollFd, EPOLL_CTL_ADD, fd, &event);
}

// used in disconnect()
int ServerManager::deleteEvent(int fd)
{
	return epoll_ctl(this->epollFd, EPOLL_CTL_DEL, fd, 0);
}

// used in loop()
int ServerManager::modifyEvent(int fd, struct epoll_event &event, int option)
{
	event.events = option;
	event.data.fd = fd;
	return epoll_ctl(this->epollFd, EPOLL_CTL_MOD, fd, &event);
}

void ServerManager::connect(int serverFd)
{
	struct sockaddr_in clientAddr;
	int clientLength = sizeof(clientAddr);
	int fd = accept(serverFd, (struct sockaddr *)&clientAddr, (socklen_t *)&clientLength);
	fcntl(fd, F_SETFL, O_NONBLOCK);
	this->fdInUse.insert(fd);
	std::cout << "id [" << fd << "]: "
						<< "connected\n";
	if (fd == -1)
		return;

	Connection newConnection(serverFd);
	connections.insert(std::make_pair(fd, newConnection));
	if (this->addEvent(fd, EPOLLIN) == -1)
		this->disconnect(fd);
}

void ServerManager::disconnect(int fd)
{
	this->deleteEvent(fd);
	connections.erase(fd);
	close(fd);
	std::cout << "id [" << fd << "]: "
						<< "disconnected\n";
}

int ServerManager::receive(int fd)
{
	char *buffer = this->buffer;
	int nbytes = recv(fd, buffer, BUFFER_SIZE, 0);
	if (nbytes == 0)
		return -1;
	else
	{
		std::cout << buffer << std::endl;
		// example:
		// Request& request = connections[fd].getRequest();
		// request.append(buffer);
	}
	return 0;
}

int ServerManager::send(int fd, Response &response)
{
	const char *buffer = response.getBuffer();
	std::size_t bufSize = response.getBufSize();
	int nbytes;

	nbytes = ::send(fd, buffer, bufSize, 0);
	if (nbytes > 0)
	{
		response.moveBufPosition(nbytes);
	}
	return nbytes;
}
