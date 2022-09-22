#ifndef __FT_REQUEST_MANAGER_H__
#define __FT_REQUEST_MANAGER_H__

#include <map>
#include <queue>
#include <sstream>

#include "request.hpp"

#define CR 0x0d
#define LF 0x0a

class RequestManager
{
private:
	enum ParseStage
	{
		// STAGE_ERROR = -1,
		STAGE_STARTLINE,
		STAGE_HEADER,
		STAGE_BODY,
		// STAGE_DONE
	};

	std::queue<Request> requestQueue;

	ParseStage parseStage;
	std::stringstream buf; // for startline and header
	std::map<std::string, std::string> headerbuf;
	size_t headerbufSize;
	// int statusCode;

public:
	RequestManager();
	RequestManager(const RequestManager &other);
	RequestManager &operator=(const RequestManager &other);
	~RequestManager();
	int fillBuffer(const char *octets, size_t len); // return status code
	bool isReady(); // request message가 1개 이상 parsing이 완료

	bool isEmpty();
	Request &pop();
	Request &front();
	size_t size();
	void pruneAll(void);

private:
	void setParseStage(ParseStage stage);
	bool detectSectionDelimiter(std::string &line);
	bool checkLineFinishedWithoutNewline(std::stringstream &buf);
	void doBufferEpilogue(std::string &line);
	size_t countParsedOctets(const std::string &line, const size_t &initialBufferLength);

	void pushDummyRequest(void);
	Request &getLatestRequest(void);

	void pruneBuffer(void);
	void prepareNextRequest(void);
};
/**
	 * 모두 완성된 request message만을 간주하는 method이다.
	 * 즉, parsing이 덜 끝난 request message를 보호하기 위해 따로 메소드를 만든 것.
	 */

#endif