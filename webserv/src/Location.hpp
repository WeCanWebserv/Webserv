#ifndef LOCATION_HPP_
#define LOCATION_HPP_

struct Location
{
private:
	std::string root; // default: empty,
	std::map<std::string, bool>
			allow_method; // default: {{"GET", false}, {"POST", flalse}, {"DELETE", false}}
	std::vector<std::string> indexFiles;     // default: empty
	bool auto_index;                         // default: false
	std::map<std::string, std::string> cgis; // default: empty
	std::string redirection;                 // default: empty
	size_t maxRequestBodySize;               // default: MAX_REQUEST_BODY_SIZE
};

class ConfigInfo
{
private:
	int host;                                  // default: required
	int port;                                  // default: required
	std::map<int, std::string> errorPages;     // defautl: empty
	std::map<std::string, Location> locations; // required
	int status;                                // END, BAD ... operator ! 연산 구현
};

class ConfigParser
{
private:
	std::string configPath; // 기본생성자로 객체가 생성되면 default 경로로 초기화된다.
	std::stringstream
			configStreamBuffer; // 처음에 configPath를 던져주면 StreamBuffer에 파일을 몽땅쓴다.
	ConfigInfo getConfigInfo();
}

#endif // LOCATION_HPP_