#include "Logger.hpp"
#include <ctime>

Logger::LogLevel Logger::logLevel;
std::ofstream Logger::logFile;
std::ofstream Logger::null("/dev/null");

void Logger::init(LogLevel level, const std::string &logFilePath)
{
	logLevel = level;
	logFile.open(logFilePath.c_str(), std::ofstream::app);

	if (logFile.good())
		std::clog.rdbuf(Logger::logFile.rdbuf());
	else
		std::clog << "could not open log file: " << logFilePath << std::endl;
}

std::string Logger::getTimestamp(const char *format)
{
	const int bufSize = 32;
	char buf[bufSize];
	std::string timestamp;
	std::time_t rawTime;
	std::tm *timeInfo;

	std::time(&rawTime);
	timeInfo = std::gmtime(&rawTime);
	std::strftime(buf, bufSize, format, timeInfo);
	return (std::string(buf));
}

std::string Logger::getLogLabel(LogLevel level)
{
	static const std::string label[] = {"error", "warning", "info", "debug"};

	return (std::string("[" + label[level] + "]"));
}

std::ostream &Logger::log(LogLevel level)
{
	if (level <= logLevel)
	{
		std::clog << getTimestamp() << " " << getLogLabel(level) << " ";
		return (std::clog);
	}
	return (null);
}

std::ostream &Logger::debug(const char *func, int line)
{
	return (log(LOGLEVEL_DEBUG) << func << ":" << line << " ");
}

std::ostream &Logger::info()
{
	return (log(LOGLEVEL_INFO));
}

std::ostream &Logger::warning()
{
	return (log(LOGLEVEL_WARNING));
}

std::ostream &Logger::error()
{
	return (log(LOGLEVEL_ERROR));
}

// FIX: add event format (like client_addr, method, uri, status_code, ...)
std::ostream &Logger::event()
{
	return (std::clog << getTimestamp() << " ");
}
