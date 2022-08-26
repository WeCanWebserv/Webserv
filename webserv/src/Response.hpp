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

	struct Uri
	{
		std::string originUri;
		std::string path;
		std::string query;
		std::string extension;
		bool isDirectory;
	};

	struct Body
	{
		int fd;
		bool isEOF;
		std::size_t readSize;
		std::stringstream buffer;
	};

private:
	static statusInfoType defaultInfo;
	int statusCode;
	headerType header;
	Body body;
	std::string buffer;
	std::size_t sentBytes;
	std::size_t totalBytes;
	bool isReady;
	bool isClose;

public:
	Response();
	Response(const Response &other);
	~Response();

	bool ready() const;
	bool done() const;
	void clear();

	const char *getBuffer() const;
	std::size_t getBufSize() const;
	std::size_t moveBufPosition(int nbyte);

	template<class Request, class ConfigInfo>
	int setRequest(Request &req, ConfigInfo config);
	template<class ConfigInfo>
	int setError(int code, ConfigInfo config, bool close = false);

	std::stringstream &getBodyStream();
	void setStatusCode(int code);
	void setHeader(std::string name, std::string value);
	void setBuffer();

private:
	Response &operator=(const Response &rhs);

	static statusInfoType initializeDefaultInfo();
	std::string getStatusInfo(int code) const;
	Uri createUri(const std::string &uri);
	void clearBody(Body &body);
};

#endif // !RESPONSE_HPP
