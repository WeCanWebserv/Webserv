#include "request_manager.hpp"
#include "../Logger.hpp"
#include "request_parser.hpp"
#include <exception>
#include <stdexcept>

RequestManager::RequestManager() : parseStage(STAGE_STARTLINE), headerbufSize(0)
{
	this->pushDummyRequest();
}

RequestManager::~RequestManager()
{
	this->pruneAll();
}

void RequestManager::pushDummyRequest(void)
{
	this->requestQueue.push(Request());
}

Request &RequestManager::getLatestRequest(void)
{
	if (this->requestQueue.empty())
		this->pushDummyRequest();
	return this->requestQueue.back();
}

void RequestManager::pruneBuffer(void)
{
	this->buf.clear();
	this->buf.str("");
	this->headerbufSize = 0;
	this->headerbuf.clear();
}

void RequestManager::pruneAll(void)
{
	this->pruneBuffer();
	Logger::debug(LOG_LINE) << "prune All is Called\n";
	while (!this->requestQueue.empty())
	{
		this->requestQueue.pop();
	}
	this->parseStage = STAGE_STARTLINE;
}

void RequestManager::prepareNextRequest(void)
{
	this->parseStage = RequestManager::STAGE_STARTLINE;
	this->pruneBuffer();
	this->pushDummyRequest();
}

bool RequestManager::isEmpty()
{
	return this->requestQueue.size() == 1;
}

Request &RequestManager::pop()
{
	if (this->requestQueue.empty())
		throw std::out_of_range("RequestQueue is Empty\n");
	Request &request = this->requestQueue.front();
	this->requestQueue.pop();
	return request;
}

Request &RequestManager::front()
{
	return this->requestQueue.front();
}

size_t RequestManager::size()
{
	return this->requestQueue.size() - 1;
}

bool RequestManager::isReady()
{
	return !isEmpty();
}

bool RequestManager::detectSectionDelimiter(std::string &line)
{
	if (line.length() == 0 || (line.length() == 1 && line[0] == CR))
		return true;
	return false;
}

bool RequestManager::checkLineFinishedWithoutNewline(std::stringstream &buf)
{
	return buf.eof();
}

void RequestManager::doBufferEpilogue(std::string &line)
{
	this->buf.clear();
	this->buf.str("");
	this->buf << line;
}

void RequestManager::setParseStage(ParseStage stage)
{
	this->parseStage = stage;
}

size_t RequestManager::countParsedOctets(const std::string &line, const size_t &initialBufferLength)
{
	return (line.length() - initialBufferLength + 1); // 개행까지 세어야 하므로 +1을 해줌
}

int RequestManager::fillBuffer(const char *octets, size_t octetSize)
{
	size_t octetOffset;
	octetOffset = 0;
	try
	{
		while (true) // Request Queue에 대한 loop
		{
			std::string tmp;
			size_t parsedLength;
			size_t initialLength;

			parsedLength = 0;
			initialLength = this->buf.str().length();
			tmp.append(octets + octetOffset, octetSize - octetOffset);
			this->buf << tmp;

			Logger::debug(LOG_LINE) << this->requestQueue.size() << "th request message is parsing...\n";
			Request &request = this->getLatestRequest();
			std::string line;
			while (this->parseStage != RequestManager::STAGE_BODY && std::getline(this->buf, line))
			{
				if (this->checkLineFinishedWithoutNewline(this->buf))
					break;
				parsedLength += countParsedOctets(line, initialLength);
				initialLength = 0;
				if (this->parseStage == RequestManager::STAGE_STARTLINE)
				{
					if (!this->detectSectionDelimiter(line)) // CRLF에 대해 trim을 한다.
					{
						RequestParser::startlineParser(request.getStartline(), line);
						this->setParseStage(RequestManager::STAGE_HEADER);
					}
				}
				else if (this->parseStage == RequestManager::STAGE_HEADER)
				{
					if (!this->detectSectionDelimiter(line))
					{
						// header section이 덜 끝났을 경우
						RequestParser::fillHeaderBuffer(this->headerbuf, line, this->headerbufSize);
						this->headerbufSize += line.length() + 1;
					}
					else
					{
						// header section이 모두 끝났을 경우
						RequestParser::headerParser(request.getHeader(), this->headerbuf,
																				request.getStartline().method);
						this->setParseStage(RequestManager::STAGE_BODY);
					}
				}
			}
			if (this->buf.eof())
			{
				this->doBufferEpilogue(line);
				break;
			}
			std::vector<char> bodyOctets(octets + parsedLength + octetOffset, octets + octetSize);
			ssize_t remainedCount;
			/**
			 * remainedCount < 0 : body가 덜 찼다.
			 * remainedCount = 0 : body에 딱 맞게 읽었고 다 찼다.
			 * remainedCount > 0 : body를 모두 읽고 남았다(다른 request message)
			 */
			Logger::debug(LOG_LINE) << "BodyOctet: " << std::string(bodyOctets.begin(), bodyOctets.end()) << "\n";
			if ((remainedCount =
							 RequestParser::bodyParser(request.getBody(), bodyOctets, request.getHeader())) >= 0)
			{
				RequestParser::postBodyParser(request.getBody(), request.getHeader());
				Logger::debug(LOG_LINE) << this->requestQueue.size()
																<< "th request message parsing is just completed\n";
				octetOffset = octetSize - remainedCount;
				this->prepareNextRequest();
			}
			if (remainedCount <= 0) // 남았을 경우에만 continue, 아니면 break한다.
				break;
		}
	}
	catch (int code)
	{
		Logger::debug(LOG_LINE) << "Error code: " << code << "\n";
		this->pruneAll();
		return code;
	}
	return 0;
}