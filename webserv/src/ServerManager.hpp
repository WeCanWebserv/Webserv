#ifndef SERVER_MANAGER_HPP_
#define SERVER_MANAGER_HPP_

#include <map>

class Connection;
class ConfigInfo;
class ConfigParser;

class ServerManager
{
private:
	ConfigParser parser;
	std::map<int, ConfigInfo> servers;     // int(server_socket_fd)
	std::map<int, Connection> connections; //  int(client_fd)
	char *buffer; // read 용. 이 버퍼를 request버퍼에 넣어주는 것이라고 고려
	int epollFd; // 서버소켓 다 만들었는데, epoll_fd 생성에 실패하면 어차피 불필요한 과정이기 때문에

public:
	ServerManager(const char *path); // NULL이면 기본 구성파일 경로, 아니라면 존재하는 파일인지 검증.
	~ServerManager(); // servers 이터레이터 돌면서 close(socket_fd)

	void initialize(); // this->prepareListening, this->startListening
	void loop(); // epoll event, connect나, disconnect는 요청대기 상태(반목문)안에서
			// 이뤄지므로 해당 함수를 private함수 또는 protected 메서드함수로 관리하기 위해서

protected:                 // 현재로서는 private이든, protected든 크게 상관 없음
	void prepareListening(); // socket_fd를 생성하고 server에 configInfo와 함께 맵 컨테이너에 저장.
	void startListening(); // servers를 iterator로 순회하여 listen() 함수 호출
			// 인터럽트 시그널에 시그널 핸들러를 지정하지 않아도 문제 없는 것인지 확인이 필요하다.

	void connect(int clientFd);    // accept, epoll_ctl, map.insert(containers에 데이터 추가)
	void toNonBlocking(int fd);    // fcntl, non-block I/O 로 바꾸기. connect에서 쓰임
	void disconnect(int clientFd); // close, epoll_ctl, map.erase

	receive(); // recv, 이벤트가 확인되면 클라이언트 fd를 인자로 recv를 호출하고 buffer를 connection을 통해 접근가능한
						 // request객체내 stringstream타입 버퍼에 insert(append)해준다.
	send(); // send, 이벤트가 확인되면 request 객체에 담긴 버퍼를 클라이언트 fd에 send()한다.

private:
	ServerManager();                                 // EQUAL DELETE
	ServerManager &operator=(const ServerManager &); // EQUAL DELETE
	ServerManager(const ServerManager &);            // EQUAL DELETE
};

void loop()
{
	struct epoll_event event; // <-- 지역변수로 가지고 있는 것이 적절한 듯.
	struct epoll_event events[MAX_EVENTS]; // vector로 data멤버로 가지고 있을까? 고민중..
	//  쓴다면 서버매니저 생성자에서 reserve(MAX_EVENTS) 사용
	// int epollfd; -> 데이터 멤버

	// playground 에서 작성한 코드
	// ev.events = EPOLLIN;
	// ev.data.fd = this->sock;
	// if (epoll_ctl(epollfd, EPOLL_CTL_ADD, this->sock, &ev) == -1)
	// {
	// 	std::cerr << "Fatal error: epoll_ctl" << std::endl;
	// 	exit(1);
	// }
	// => servers(map<server_fd, server_config>) 순회하여 epoll이벤트 추가하기.

	int nfds;
	for (;;)
	{
		// try, catch exception 의 적극적인 사용을 고려한다면 epoll_wait을 wait과 같은 함수로 래핑하고
		// 예외 상횡에 대해서 catch 블러에서 처리해도 괜찮...을까?
		nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		if (nfds == -1)
		{
			std::cerr << "Fatal error: epoll_wait" << std::endl;
			exit(1);
		}

		for (int i = 0; i < nfds; i++)
		{
			int eventFd = events[i].data.fd;
			int eventsFlag = events[i].event;
			if (eventFd == this->sock)
			{
				this->connect() // {
				// 	int newClientFd = accept(this->sock, (struct sockaddr *)&this->csin, &this->cslen);
				// 	if (newClientFd == -1)
				// 	{
				// 		std::cerr << "Fatal error: accept" << std::endl;
				// 		exit(1);
				// 	}
				// 	fcntl(newClientFd, F_SETFL, O_NONBLOCK);
				// 	ev.events = EPOLLIN | EPOLLET;
				// 	ev.data.fd = newClientFd;
				// 	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, newClientFd, &ev) == -1)
				// 	{
				// 		std::cerr << "Fatal error: epoll_ctl" << std::endl;
				// 		exit(1);
				// 	}
				// 	this->mapOfClients[newClientFd] = this->nextId++;
				// 	std::cout << "server: client " << mapOfClients[newClientFd] << " just arrived" << std::endl;
				// }
			}
			else // disconnect가 반복되니 catch block에서 처리하는 것에 대해 고민해볼 수 있을 듯.
			{
				Connection &connection = this->connections[eventFd];
				if (/*exception 확인*/)
				{
					// handleExecption...
					disconnect(eventFd);
				}
				else if (/*write event 확인 && */)
				{
					Response &response = connection.getResponse();
					if (!response) // 상태 확인
					{
						// 파일 또는 파이프fd에 대한 읽기/쓰기는 response에서 관리하고 서버매니저는 socket_fd에 대한 읽기 쓰기만..!
						// CGI WRITE가 내부 플래그(?)를 통해 확인될 수 있음.
						// .... 로직 ...
						// 또는 파싱된 getResponse를 완성된 메시지를 저장하는 버퍼에 쓰는 ...
					}
					else
						(!this->send(eventFd)) // send한 사이즈가 0이라면
																	 // 또는 this->send(eventFd, response);
						{
							// keep-allive를 체크해서 바로 끊어줄지 안끊어줄지 고민할듯.
							// 유지한다면, request. response의 파싱된 데이터를 clear()해주는 함수가 필요해보인다.
						}
				}
			}
			else(/*read event 확인 && */)
			{
				// read 반환값이 0이 라면 -1로 반환처리하면 보통의 시스템콜 래퍼함수의 에러처리와 일치시킬 수 있음.
				if (this->recieve(eventFd) < 0)
					disconnect();

				Request &request = connection.getRequest();
				if (!request) // 상태 확인
					continue;
				Response &response = connection.getResponse();
				response.populate(request);
				// epoll_ctl, eventFd에 대한 쓰기 이벤트 추가.
			}
			// eventFd에 대한 쓰기 이벤트 추가.
			// 클라이언트 측에서 request 메시지 보낼 때까지 기다리면 사실 버퍼가 하나만 있어도 될 것 같은데...
			// response 버퍼 작업 하는 도중에 클라이언트 측에 read이벤트 발생하면, 기존 response를 clear()
			// 해주는 상황에 대해서 고려해볼 수 있을 것 같다. clear() 해줄 것인가 아니면.. 처리될 때까지 읽지 않을 것인가.
		}
	}
}
// void prepareListening()
// {
// 	while (1)
// 	{
// 		ConfigInfo info = configParser.getConfigInfo();
// 		if (!info)
// 			break;
// 		configInfos.push_back(info);
// 	}
// }

#endif // SERVER_MANAGER_HPP_