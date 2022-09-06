#ifndef MEDIA_TYPE_HPP
#define MEDIA_TYPE_HPP

#include <map>
#include <string>

class MediaType
{
public:
	typedef std::map<std::string, std::string> mimeType;

private:
	static mimeType mime;

public:
	static std::string get(const std::string &extension);

private:
	MediaType();
	MediaType(const MediaType &other);
	MediaType &operator=(const MediaType &rhs);
	~MediaType();

	static mimeType initMime();
};

#endif // !MEDIA_TYPE_HPP
