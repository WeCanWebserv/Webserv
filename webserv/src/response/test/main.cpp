#include "../Response.cpp"
#include "../Response.hpp"
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

struct Request
{
	std::string uri;
	std::string method;
};

struct Location
{
	std::set<std::string> allowMethod;
	std::map<std::string, std::string> cgis;
	std::vector<std::string> indexFiles;
	std::string root;
	bool isAutoIndex;
};

struct Config
{
	typedef std::map<std::string, Location> locationType;
	std::map<std::string, Location> location;
	std::map<int, std::string> errorPages;
};

Request req;
Location loc;
Config conf;

void initRequest(const std::string &method, const std::string &uri)
{
	req.method = method;
	req.uri = uri;
}

void testResponseBuffer(std::string method, std::string uri)
{
	Response res;

	std::cout << "<< " << method << " " << uri << std::endl;
	std::cout << std::endl;
	initRequest(method, uri);

	// 타겟 파일의 fd를 구한다 혹은 버퍼에 본문을 직접 쓴다
	try
	{
		res.process(req, conf);
	}
	catch (int statusCode)
	{
		res.process(statusCode, conf, true);
	}
	catch (...)
	{}

	// 타겟 fd를 끝까지 읽는다
	if (!res.ready())
	{
		int n;

		while (true)
		{
			n = res.readBody();
			if (n == 0)
				break;
			if (n == -1)
			{
				std::cerr << "read error" << std::endl;
				return;
			}
		}
	}

	// response가 비워질 때까지 보낸다
	// 아래에서는 출력을 위해 개행을 기준으로 나눠 보내고 있다
	while (!res.done())
	{
		const char *buf = res.getBuffer();
		const char *pos = std::strchr(buf, '\n');
		int n = pos ? (pos + 1) - buf : res.getBufSize();

		std::cout.write(buf, n);
		res.moveBufPosition(n);
	}
}

int main()
{
	std::string repo;

	// repo = "/your/absolute/path/";
	repo = "/Users/seushin/projects/42cursus/Webserv";
	loc.root = repo + "/webserv/src/response/test/";
	loc.allowMethod.insert("GET");
	loc.indexFiles.push_back("index.html");
	loc.isAutoIndex = false;

	conf.location["/"] = loc;
	conf.errorPages[404] = "./404.html";

	testResponseBuffer("GET", "/index.html");
	std::cout << std::string(50, '=') << std::endl;

	testResponseBuffer("GET", "/");
	std::cout << std::string(50, '=') << std::endl;

	conf.location["/"].isAutoIndex = true;
	testResponseBuffer("GET", "/");
	std::cout << std::string(50, '=') << std::endl;

	testResponseBuffer("POST", "/index.html");
	std::cout << std::string(50, '=') << std::endl;

	return (0);
}
