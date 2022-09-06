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
#include "Connection.hpp"
#include "response/Response.hpp"

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
			 connIter != this->connections.end(); connIter++)
	{
		close(connIter->first);
	}
	close(this->epollFd);
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

						if (request.ready())
						{
							newEvent = response.process(request, servers[eventFd]);
						}
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
					// TODO: cgi
					int rPipe = 0;
					int n = 0;

					if (n <= 0)
					{
						this->deleteEvent(eventFd);
						close(eventFd);
						this->addEvent(rPipe, EPOLLIN);
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
int ServerManager::addEvent(int clientFd, int option)
{
	struct epoll_event event;
	event.events = option;
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
		return;

	Connection newConnection(serverFd);
	connections.insert(std::make_pair(clientFd, newConnection));
	if (this->addEvent(clientFd, EPOLLIN) == -1)
		this->disconnect(clientFd);
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
