#ifndef URI_PARSER_HPP
#define URI_PARSER_HPP

#include <string>

class UriParser
{
private:
	const std::string uri;

public:
	UriParser(const std::string &uri);
	~UriParser();

	std::string getPath() const;
	std::string getQuery() const;
	std::string getExtension() const;
	std::string getFile() const;
	std::string getPathInfo() const;
	bool isDirectory() const;

private:
	UriParser();
	UriParser(const UriParser &other);
	UriParser &operator=(const UriParser &rhs);

	bool isPrefixWith(const std::string &str, const std::string &prefix) const;
	bool isSuffixWith(const std::string &str, const std::string &suffix) const;
	std::string preprocess() const;
};

#endif // !URI_PARSER_HPP
