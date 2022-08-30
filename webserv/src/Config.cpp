#include "Config.hpp"


class ConfigParser
{
    public:
        struct MainConfig test()
        {
            MainConfig config;

			HttpConfig &http = config.httpConfig;
			ServerConfig &server = http.tableOfServers[42];
			{
				server.tableOfHosts.insert("127.0.0.1:8080");
				server.tableOfHosts.insert("127.0.0.1:4242");

				server.listOfServerNames.push_back("example.com");
				server.listOfServerNames.push_back("www.example.com");

				server.maxRequestBodySize = 10240;
				{
					LocationConfig& location = server.tableOfLocations["/"];

					location.root = "/usr/src/webserv";

					location.indexFiles.push_back("index.html");
					location.indexFiles.push_back("index.htm");

					location.tableOfCgiBins.insert(std::make_pair(".php", "/usr/local/opt/php"));
					location.tableOfCgiBins.insert(std::make_pair(".py", "/usr/local/opt/python@3.10"));

					location.tableOfCgiUploads.insert(std::make_pair(".py", "/usr/local/upload"));

					location.tableOfErrorPages.insert(std::make_pair(501, "/error/50x.html"));
					location.tableOfErrorPages.insert(std::make_pair(502, "/error/50x.html"));
				}
			}
            return config;
        }
};

int main()
{
	ConfigParser parser;
	MainConfig config = parser.test();
	config.print();
	return 0;
}
