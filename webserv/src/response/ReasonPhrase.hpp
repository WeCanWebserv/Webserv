#ifndef REASON_PHRASE_HPP
#define REASON_PHRASE_HPP

#include <map>
#include <string>

class ReasonPhrase
{
public:
	typedef std::map<int, std::string> reasonType;

private:
	static reasonType reason;

public:
	static std::string get(int statusCode);

private:
	ReasonPhrase();
	ReasonPhrase(const ReasonPhrase &other);
	ReasonPhrase &operator=(const ReasonPhrase &rhs);
	~ReasonPhrase();

	static reasonType initReason();
};

#endif // !REASON_PHRASE_HPP
