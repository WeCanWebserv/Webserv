#include "UriParser.hpp"

UriParser::UriParser(const std::string &uri) : uri(uri) {}

UriParser::~UriParser() {}

std::string UriParser::preprocess() const
{
	std::string path = uri;
	std::string::size_type pos;

	if (isPrefixWith(path, "http://") || isPrefixWith(path, "https://"))
	{
		/**
		 * https://www.example.com/index.html
		 *                        ^
		 *                        find here
		 */
		pos = path.find('/', path.find(':') + 3);
		if (pos == std::string::npos)
			pos = path.size();
		path = path.substr(pos);
	}

	if (!isPrefixWith(path, "/"))
		path.insert(path.begin(), '/');

	pos = path.find("..");
	while (pos != std::string::npos)
	{
		path.replace(pos, 2, "");
		pos = path.find("..");
	}

	return (path);
}

std::string UriParser::getPath() const
{
	std::string path;

	path = preprocess();

	path = path.substr(0, path.find('?'));
	path = path.substr(0, path.find('#'));

	return (path);
}

std::string UriParser::getQuery() const
{
	std::string path;
	std::string::size_type pos;

	path = preprocess();

	pos = path.find('?');
	if (pos == std::string::npos)
		return ("");
	return (path.substr(pos + 1));
}

std::string UriParser::getExtension() const
{
	std::string path;
	std::string::size_type pos;

	path = getPath();

	pos = path.find('.');
	if (pos == std::string::npos)
		return ("");
	return (path.substr(pos));
}

bool UriParser::isDirectory() const
{
	std::string path;

	path = getPath();
	return (isSuffixWith(path, "/"));
}

bool UriParser::isPrefixWith(const std::string &str, const std::string &prefix) const
{
	const int strSize = str.size();
	const int preSize = prefix.size();

	if (strSize >= preSize)
	{
		if (str.compare(0, preSize, prefix) == 0)
			return (true);
	}
	return (false);
}

bool UriParser::isSuffixWith(const std::string &str, const std::string &suffix) const
{
	const int strSize = str.size();
	const int sufSize = suffix.size();

	if (strSize >= sufSize)
	{
		if (str.compare(strSize - sufSize, sufSize, suffix) == 0)
			return (true);
	}
	return (false);
}
