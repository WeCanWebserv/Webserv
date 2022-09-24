#include "ServerManager.hpp"

#include <arpa/inet.h>
#include <csignal>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <exception>
#include <map>
#include <utility>
#include <vector>

#include "Config.hpp"
#include "ConfigParser.hpp"

#include "Connection.hpp"
#include "Logger.hpp"
#include "response/Response.hpp"

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
			setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, (void *)&optVal, (socklen_t)sizeof(optVal));
		}

		ServerConfig serverConfig = serverConfigs[idx];
		sockAddr.sin_addr.s_addr = inet_addr(serverConfig.listennedHost.c_str());
		sockAddr.sin_port = htons(atoi(serverConfig.listennedPort.c_str()));
		Logger::debug(LOG_LINE) << "host: " << serverConfig.listennedHost
														<< ", port: " << atoi(serverConfig.listennedPort.c_str()) << std::endl;
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

	signal(SIGCHLD, SIG_IGN);
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
			server_container_type::iterator foundServer = this->servers.find(eventFd);
			if (foundServer != this->servers.end() && occurredEvent & EPOLLIN)
			{
				this->connect(eventFd);
				continue;
			}

			/**
			 * 클라이언트 소켓 이벤트
			 * HTTP Request 혹은 HTTP Response
			 */
			connection_container_type::iterator foundConn = this->connections.find(eventFd);
			if (foundConn != this->connections.end())
			{
				Connection &connection = foundConn->second;
				RequestManager &requestManager = connection.getRequestManager();
				Response &response = connection.getResponse();

				if (occurredEvent & EPOLLERR || occurredEvent & EPOLLHUP)
				{
					Logger::error() << "ServerManager: " << eventFd << ": occurred error event" << std::endl;
					this->disconnect(eventFd);
				}
				else if (occurredEvent & EPOLLIN)
				{
					const ServerConfig &config = this->servers[connection.getServerFd()];
					std::pair<int, int> pipeEvent(-1, -1);

					try
					{
						if (this->receive(eventFd) == -1)
						{
							this->disconnect(eventFd);
							continue;
						}
						if (requestManager.isReady())
						{
							Request request = requestManager.pop();
							pipeEvent = response.process(request, config, eventFd);
							registerResposneEvent(eventFd, response, pipeEvent);
						}
					}
					catch (int errorCode)
					{
						response.process(errorCode, config);
						registerResposneEvent(eventFd, response, pipeEvent);
					}
					catch (const std::exception &e)
					{
						Logger::error() << e.what() << std::endl;
						this->disconnect(eventFd);
						continue;
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
						{
							Logger::info() << "response done. connection close" << std::endl;
							this->disconnect(eventFd);
						}
						else
						{
							connection.increaseTransactionCount();

							if (connection.getTransactionCount() + 1 >= Connection::maxTransaction)
								response.setClose();

							if (requestManager.isReady())
							{
								const ServerConfig &config = this->servers[connection.getServerFd()];
								std::pair<int, int> pipeEvent(-1, -1);

								Logger::info() << "handle pipelining request" << std::endl;
								try
								{
									Request request = requestManager.pop();
									pipeEvent = response.process(request, config, eventFd);
									registerResposneEvent(eventFd, response, pipeEvent);
								}
								catch (int errorCode)
								{
									response.process(errorCode, config);
									registerResposneEvent(eventFd, response, pipeEvent);
								}
							}
							else if (this->modifyEvent(eventFd, currentEvent, EPOLLIN) != -1)
							{
								Logger::info() << "keep-alive, clear connection" << std::endl;
								connection.clear(); // use request.clear(), response.claer())
							}
							else
							{
								Logger::info() << "ServerManager: " << std::strerror(errno) << std::endl;
								this->disconnect(eventFd);
							}
						}
					}
				}
				connection.setLastAcceesTime(time(NULL));
			}
			else
			{
				/**
				 * HTTP Response에 해당하는 파일/cgi 이벤트
				 */
				extra_fd_container_type::iterator foundFd = this->extraFds.find(eventFd);
				connection_container_type::iterator foundConn = this->connections.find(foundFd->second);

				if (foundFd == this->extraFds.end() || foundConn == this->connections.end())
				{
					Logger::error() << "undefined extra fd" << std::endl;
					this->deleteEvent(eventFd);
					this->extraFds.erase(eventFd);
					close(eventFd);
					continue;
				}

				Connection &connection = foundConn->second;
				Response &response = connection.getResponse();
				int originFd = foundFd->second;

				if (occurredEvent & EPOLLHUP)
				{
					try
					{
						response.parseCgiResponse();
					}
					catch (int errorCode)
					{
						const ServerConfig &config = this->servers[connection.getServerFd()];
						response.process(errorCode, config, true);
					}

					/**
					 * cgi 스크립트가 응답을 완료하여 이벤트 삭제
					 */
					this->deleteEvent(eventFd);
					this->extraFds.erase(eventFd);
					close(eventFd);

					/**
					 * client에게 Response send를 위한 이벤트 등록
					 */
					this->modifyEvent(originFd, currentEvent, EPOLLIN | EPOLLOUT);
				}
				else if (occurredEvent & EPOLLIN)
				{
					int n;

					n = response.readPipe();
					if (n < 0)
					{
						const ServerConfig &config = this->servers[connection.getServerFd()];
						response.process(503, config, true);

						this->deleteEvent(eventFd);
						this->extraFds.erase(eventFd);
						close(eventFd);

						this->modifyEvent(originFd, currentEvent, EPOLLIN | EPOLLOUT);
					}
				}
				else if (occurredEvent & EPOLLOUT)
				{
					int rdPipe = response.writePipe();

					if (rdPipe == 0)
						continue;

					this->deleteEvent(eventFd);
					this->extraFds.erase(eventFd);
					close(eventFd);

					if (rdPipe < 0)
					{
						const ServerConfig &config = this->servers[connection.getServerFd()];
						response.process(503, config, true);

						this->modifyEvent(originFd, currentEvent, EPOLLIN | EPOLLOUT);
					}
					else
					{
						this->addEvent(rdPipe, EPOLLIN);
						extraFds[rdPipe] = originFd;
					}
				}
			}
		}
		for (connection_container_type::iterator connIter = connections.begin();
				 connIter != connections.end(); connIter++)
		{
			int clientFd = connIter->first;
			Connection &connection = connIter->second;

			if (connection.checkTimeOut())
			{
				this->disconnect(clientFd);

				Response &response = connection.getResponse();
				if (response.isCgi())
				{
					std::pair<int, int> pipeFds = response.killCgiScript();
					int targetFd;

					if (this->extraFds.find(pipeFds.first) != this->extraFds.end())
						targetFd = pipeFds.first;
					else
						targetFd = pipeFds.second;
					this->deleteEvent(targetFd);
					this->extraFds.erase(targetFd);

					close(pipeFds.first);
					close(pipeFds.second);
				}
			}
		}
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
	if (fd == -1)
	{
		Logger::error() << "accept: " << serverFd << " " << std::strerror(errno) << std::endl;
		return;
	}
	fcntl(fd, F_SETFL, O_NONBLOCK);
	this->fdInUse.insert(fd);

	if (this->connections.find(fd) != this->connections.end())
	{
		this->connections[fd].clear();
	}
	this->connections[fd].setServerFd(serverFd);
	this->connections[fd].setMaxBodySize(this->servers[serverFd].maxRequestBodySize);
	Logger::info() << "ServerManager: " << fd << " connected" << std::endl;
	if (this->addEvent(fd, EPOLLIN) == -1)
	{
		Logger::error() << "ServerManager::connect: " << std::strerror(errno) << std::endl;
		this->disconnect(fd);
	}
}

void ServerManager::disconnect(int fd)
{
	this->deleteEvent(fd);
	this->connections.erase(fd);
	close(fd);
	Logger::info() << "ServerManager: " << fd << " disconnected" << std::endl;
}

int ServerManager::receive(int fd)
{
	char *buffer = this->buffer;
	int nbytes = recv(fd, buffer, BUFFER_SIZE, 0);
	if (nbytes == 0)
		return -1;
	else if (nbytes < 0)
	{
		Logger::error() << fd << ": recv failure" << std::endl;
		return -1;
	}
	else
	{
		RequestManager &requestManager = connections[fd].getRequestManager();
		requestManager.fillBuffer(buffer, nbytes);
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
	else if (nbytes < 0)
	{
		Logger::error() << fd << ": send failure" << std::endl;
	}
	return nbytes;
}

void ServerManager::registerResposneEvent(int eventFd, Response &res, std::pair<int, int> pipeEvent)
{
	epoll_event dummyEvent;

	if (pipeEvent.first != -1)
	{
		this->extraFds[pipeEvent.first] = eventFd;
		if (this->addEvent(pipeEvent.first, pipeEvent.second) == -1)
		{
			Logger::debug(LOG_LINE) << std::strerror(errno) << ": errno " << errno << std::endl;
			this->disconnect(eventFd);
			return;
		}
		Logger::debug(LOG_LINE) << "add pipe event: fd " << pipeEvent.first << std::endl;
	}
	else if (res.ready())
	{
		if (this->modifyEvent(eventFd, dummyEvent, EPOLLIN | EPOLLOUT) == -1)
			this->disconnect(eventFd);
		Logger::debug(LOG_LINE) << "modify client event to EPOLLIN | EPOLLOUT" << std::endl;
	}
}
