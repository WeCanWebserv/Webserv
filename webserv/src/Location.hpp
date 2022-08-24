#ifndef LOCATION_HPP_
#define LOCATION_HPP_

#include <map>
#include <sstream>
#include <string>
#include <vector>

struct Location
{
private:
	std::string root; // default: empty,
	std::map<std::string, bool> allow_method;
	// default: {{"GET", false}, {"POST", flalse}, {"DELETE", false}}
	std::vector<std::string> indexFiles;     // default: empty
	bool auto_index;                         // default: false
	std::map<std::string, std::string> cgis; // default: empty
	std::string redirection;                 // default: empty
	size_t maxRequestBodySize;               // default: MAX_REQUEST_BODY_SIZE
};

#endif // LOCATION_HPP_