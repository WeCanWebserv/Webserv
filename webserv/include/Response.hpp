#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <map>
#include <sstream>
#include <string>

class Response
{
public:
	typedef std::map<int, std::string> statusInfoType;
	typedef std::map<std::string, std::string> headerType;

private:
	static statusInfoType defaultInfo;
	int statusCode;
	headerType header;
	std::stringstream body;
	std::string buffer;
	std::size_t sentBytes;
	std::size_t totalBytes;
	bool isReady;

public:
	Response();
	Response(const Response &other);
	Response &operator=(const Response &rhs);
	~Response();
};

#endif // !RESPONSE_HPP
