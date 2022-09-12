#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "Config.hpp"

struct LocationConfig
{
	std::string root;
	std::vector<std::string> indexFiles;
	std::map<std::string, std::string> tableOfCgiBins;
	std::map<std::string, std::string> tableOfCgiUploads;
	std::pair<int, std::string> redirectionSetting;
	std::set<std::string> allowedMethods;
	bool isAutoIndexOn;
	bool isRedirectionSet;  // 'return' 지시어 중복 방지
};
// Example 1:
// location /download {
//      root html;
//      index index.html index.htm;
//      autoindex on;  // default = off
//      cgi_bin .php /usr/bin/php;
//      cgi_upload .php /usr/local/download;
//      allowed_methods GET POST DELETE;  // default = GET
// }

// Example 2:
// location /redir {
//     return 301 /usr/upload;
// }


struct ServerConfig
{
	std::vector<std::string> listOfServerNames;
	std::map<int, std::string> tableOfErrorPages;
	std::map<std::string, LocationConfig> tableOfLocations;
	int listennedHost;
	int listennedPort;
	int maxRequestBodySize;
};
// Example 1:
// server {
//     listen 2130706433 80; <-- To Fix
//     server_name example.com;

//     location {...} {
//        {...}
//     }
//     location {...} {
//        {...}
//     }
// 
//     max_request_body_size 10240;
//     error_page 500 502 503 504  /50x.html;
// }

struct Config
{
	std::vector<ServerConfig> serverConfigs;
};

#endif // CONFIG_HPP
