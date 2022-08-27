#ifndef LOCATION_HPP_
#define LOCATION_HPP_

#include <map>
#include <sstream>
#include <string>
#include <vector>

// class LocationParser
// {
// 	getLocation
// };

struct CgiInfo{
	std::string extension;
	std::string binPath;
	std::string uploadPath;
};

class Location
{
public:
	std::string path;
	std::string root; // default: empty,
	std::map<std::string, bool> allowedMethods;
	// default: {{"GET", false}, {"POST", flalse}, {"DELETE", false}}
	std::vector<std::string> indexFiles;     // default: empty
	bool autoIndexOn;                        // default: false
	std::map<std::string, std::string> cgis; // default: empty
	std::string redirection;                 // default: empty
	size_t maxRequestBodySize;               // default: MAX_REQUEST_BODY_SIZE
	CgiInfo cgiInfo;

public:
	Location();
	~Location();

	const std::string &getRoot() const;
	const std::map<std::string, bool> &getAllowedMethods() const;
	const std::vector<std::string> getIndexFiles() const;
	bool isAutoIndexOn() const;
	const std::map<std::string, std::string> &getCgis() const;
	const std::string &getRedirection() const;
	size_t getMaxRequestBodySize() const;
};

#endif // LOCATION_HPP_