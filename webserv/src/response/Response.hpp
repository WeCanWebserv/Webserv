#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <ctime>
#include <map>
#include <sstream>
#include <string>
#include <vector>

class Cgi;

class Response
{
public:
	typedef std::map<std::string, std::string> headerType;

	struct Body
	{
		int fd;
		std::size_t size;
		std::stringstream buffer;

		Body() : fd(-1), size(0), buffer() {}
	};

private:
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
	~Response();

	bool ready() const;
	bool done() const;
	void clear();

	const char *getBuffer() const;
	std::size_t getBufSize() const;
	std::size_t moveBufPosition(int nbyte);

	template<class Request, class ConfigInfo>
	void process(Request &req, ConfigInfo &config);
	template<class ConfigInfo>
	void process(int errorCode, ConfigInfo &config, bool close = false);

	int readBody();

private:
	Response(const Response &other);
	Response &operator=(const Response &rhs);

	std::string timeInfoToString(std::tm *timeInfo, const std::string format) const;
	std::string getCurrentTime() const;

	template<class Locations>
	typename Locations::iterator findLocation(std::string path, Locations &location);

	void setStatusCode(int code);
	void setHeader(std::string name, std::string value);
	void setBuffer();

	void setBodyToDefaultPage(const std::string &html);
	std::string generateDefaultErrorPage(int code) const;
	std::string generateFileListPage(
			const std::string &path, const std::vector<std::string> &files) const;

	void clearBody(Body &body);

	std::vector<std::string> readDirectory(const std::string &path);
	std::string searchIndexFile(
			const std::vector<std::string> &files, const std::vector<std::string> &indexFiles);

	friend Cgi;
};

#endif // !RESPONSE_HPP
