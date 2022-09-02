#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <fstream>
#include <iostream>
#include <string>

#define LOG_LINE __func__,  __LINE__

class Logger
{
public:
	enum LogLevel
	{
		LOGLEVEL_ERROR,
		LOGLEVEL_WARNING,
		LOGLEVEL_INFO,
		LOGLEVEL_DEBUG,
	};
	static LogLevel logLevel;
	static std::ofstream logFile;
	static std::ofstream null;

public:
	static void init(LogLevel level, const std::string &logFilePath);

	static std::ostream &log(LogLevel level);
	static std::ostream &debug(const char *func = "no func", int line = 0);
	static std::ostream &info();
	static std::ostream &warning();
	static std::ostream &error();
	static std::ostream &event();

	static std::string getTimestamp(const char *format = "%F %T +0000");
	static std::string getLogLabel(LogLevel level);

private:
	Logger();
	Logger(const Logger &other);
	~Logger();
	Logger &operator=(const Logger &rhs);
};

#endif // !LOGGER_HPP
