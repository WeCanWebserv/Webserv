#include "../UriParser.hpp"
#include "../../Logger.hpp"
#include <string>

void expect(std::string a, std::string b)
{
	if (a != b)
		Logger::error() << " expected " << b << " but value is " << a << "\n";
	else
		Logger::info() << a << " = " << b << "\n";

}

int main()
{
	Logger::init(Logger::LOGLEVEL_DEBUG, "/dev/stderr");
	{
		std::string uri = "/index.html?query=string";
		UriParser uriParser(uri);

		expect(uriParser.getPath(), "/index.html");
		expect(uriParser.getExtension(), ".html");
		expect(uriParser.getQuery(), "query=string");
		expect(uriParser.getFile(), "/index.html");
		expect(uriParser.getPathInfo(), "");
	}
	{
		std::string uri = "/index.html/extra/path?query=string";
		UriParser uriParser(uri);

		expect(uriParser.getPath(), "/index.html/extra/path");
		expect(uriParser.getExtension(), ".html");
		expect(uriParser.getQuery(), "query=string");
		expect(uriParser.getFile(), "/index.html");
		expect(uriParser.getPathInfo(), "/extra/path");
	}
	{
		std::string uri = "/path/file";
		UriParser uriParser(uri);

		expect(uriParser.getPath(), "/path/file");
		expect(uriParser.getExtension(), "");
		expect(uriParser.getQuery(), "");
		expect(uriParser.getExtension(), "");
		expect(uriParser.getFile(), "/path/file");
		expect(uriParser.getPathInfo(), "");
	}
	return (0);
}
