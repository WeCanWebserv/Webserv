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

		Uri(std::string uri)
		{
			std::string::size_type pos;

			this->originUri = uri;
			pos = uri.find("://");
			if (!isPrefixWith(uri, "/") && pos != std::string::npos)
			{
				pos = uri.find('/', pos + 1);
				path = pos == std::string::npos ? "/" : uri.substr(pos);
			}
			else
				path = uri;

			pos = path.find("?");
			if (pos != std::string::npos)
			{
				query = path.substr(pos + 1);
				path = path.substr(0, pos);
			}

			pos = path.rfind(".");
			if (pos != std::string::npos)
			{
				extension = path.substr(pos);
			}
		}
	};

	struct Body
	{
		int fd;
		bool isEOF;
		std::size_t readSize;
		std::stringstream buffer;

		void clear()
		{
			this->fd = -1;
			this->isEOF = false;
			readSize = 0;
			buffer.str("");
		}
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
};

#endif // !RESPONSE_HPP
