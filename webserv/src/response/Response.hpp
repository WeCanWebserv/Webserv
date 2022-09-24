#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "../Config.hpp"
#include "../request/request.hpp"
#include "Cgi.hpp"

#include <ctime>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

class Response
{
public:
	typedef std::map<std::string, std::string> headerType;

	struct Body
	{
		int fd;
		std::size_t size;
		std::stringstream buffer;
		std::ifstream file;

		Body() : fd(-1), size(0), buffer(), file() {}
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
	Cgi cgi;

public:
	Response();
	Response(const Response &other);
	~Response();

	bool ready() const;
	bool done() const;
	bool close() const;
	void clear();

	void setClose();

	const char *getBuffer() const;
	std::size_t getBufSize() const;
	std::size_t moveBufPosition(int nbyte);

	std::pair<int, int> process(Request &req, const ServerConfig &config, int clientFd);
	void process(int errorCode, const ServerConfig &config, bool close = true);

	int readPipe();
	int writePipe();
	void parseCgiResponse();
	std::pair<int, int> killCgiScript();

private:
	std::map<std::string, LocationConfig>::const_iterator
	findLocation(std::string path, const std::map<std::string, LocationConfig> &location);

	void setStatusCode(int code);
	void setHeader(std::string name, std::string value);
	void setBuffer();

	std::pair<int, int> setRedirect(int status, const std::string &location);

	std::pair<int, int> setBodyToDefaultPage(const std::string &html);
	std::string generateDefaultErrorPage(int code) const;
	std::string generateFileListPage(const std::string &uri,
																	 const std::string &root,
																	 const std::vector<std::string> &files) const;

	void clearBuffer();
	void clearBody(Body &body);

	size_t getFileSize();

	bool isDirectory(const std::string &path);
	std::vector<std::string> readDirectory(const std::string &path);
	std::string searchIndexFile(const std::vector<std::string> &files,
															const std::vector<std::string> &indexFiles);

	std::string timeInfoToString(std::tm *timeInfo, const std::string format) const;
	std::string getCurrentTime() const;

	friend class Cgi;
};

#endif // !RESPONSE_HPP
