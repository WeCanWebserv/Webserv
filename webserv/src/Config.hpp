#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

template<class T>
void printList(const std::string &description, const std::vector<T> &list)
{
	std::cout << description << std::endl;
	for (size_t i = 0; i < list.size(); i++)
	{
		std::cout << "\t[" << i << "]: " << list[i] << std::endl;
	}
	std::cout << std::endl;
}

template<class T>
void printSet(const std::string &description, const std::set<T> &set)
{
	std::cout << description << std::endl;
	int i = 0;
	for (typename std::set<T>::const_iterator start = set.begin(); start != set.end(); start++)
	{
		std::cout << "\t[" << i++ << "]: " << *start << std::endl;
	}
	std::cout << std::endl;
}

template<class K, class V>
void printTable(const std::string &description, const std::map<K, V> table)
{
	std::cout << description << std::endl;
	for (typename std::map<K, V>::const_iterator start = table.begin(); start != table.end(); start++)
	{
		std::cout << "\t[" << start->first << "]: " << start->second << std::endl;
	}
	std::cout << std::endl;
}

struct LocationConfig
{
	std::string root;
	std::vector<std::string> indexFiles;                  // index files...
	std::map<std::string, std::string> tableOfCgiBins;    // .extension, bin_path
	std::map<std::string, std::string> tableOfCgiUploads; // .extention, upload_path
	std::map<int, std::string> tableOfErrorPages;         // reponse_status, erorr_page_path

	void print()
	{
		std::cout << "Location Configuration" << std::endl;
		printList("Index Files: ", this->indexFiles);
		printTable("CGI Support: ", this->tableOfCgiBins);
		printTable("CGI Uploads: ", this->tableOfCgiUploads);
		printTable("Error Pages: ", this->tableOfErrorPages);
	}
};

struct ServerConfig
{
	std::set<std::string> tableOfHosts;                     // addr:port
	std::vector<std::string> listOfServerNames;             // server_names
    int maxRequestBodySize;
	std::map<std::string, LocationConfig> tableOfLocations; // uri, location_block

	void print()
	{
		std::cout << "ServerConfiguration" << std::endl;
        std::cout << "maxRquestBodySize: " << maxRequestBodySize << std::endl;
		printSet("address:port : ", this->tableOfHosts);
		printList("Server Names: ", this->listOfServerNames);

		for (std::map<std::string, LocationConfig>::iterator start = tableOfLocations.begin();
				 start != tableOfLocations.end(); start++)
		{
			std::cout << "[" << start->first << "] ";
			start->second.print();
		}
	}
};

struct HttpConfig
{
	std::map<int, ServerConfig> tableOfServers; // socket_fd, server_block
	// std::string defaultType;
	// int keepAliveTimeout;

	void print()
	{
		for (std::map<int, ServerConfig>::iterator start = tableOfServers.begin();
				 start != tableOfServers.end(); start++)
		{
			std::cout << "[" << start->first << "] ";
			start->second.print();
		}
	}
};

struct MainConfig
{
	const std::string info;
	HttpConfig httpConfig;

	MainConfig() : info("wcw/0.0.1") {}

	void print()
	{
		std::cout << "Server info/version: " << this->info << std::endl;
		std::cout << std::endl;

		httpConfig.print();
	}
};