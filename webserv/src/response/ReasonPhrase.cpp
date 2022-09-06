#include "ReasonPhrase.hpp"

ReasonPhrase::reasonType ReasonPhrase::reason = initReason();

ReasonPhrase::reasonType ReasonPhrase::initReason()
{
	reasonType reason;

	reason[100] = "Continue";
	reason[101] = "Switching Protocols";
	reason[200] = "OK";
	reason[201] = "Created";
	reason[202] = "Accepted";
	reason[203] = "Non-Authoritative Information";
	reason[204] = "No Content";
	reason[205] = "Reset Content";
	reason[206] = "Partial Content";
	reason[300] = "Multiple Choices";
	reason[301] = "Moved Permanently";
	reason[302] = "Found";
	reason[303] = "See Other";
	reason[304] = "Not Modified";
	reason[305] = "Use Proxy";
	reason[307] = "Temporary Redirect";
	reason[400] = "Bad Request";
	reason[401] = "Unauthorized";
	reason[402] = "Payment Required";
	reason[403] = "Forbidden";
	reason[404] = "Not Found";
	reason[405] = "Method Not Allowed";
	reason[406] = "Not Acceptable";
	reason[407] = "Proxy Authentication Required";
	reason[408] = "Request Timeout";
	reason[409] = "Conflict";
	reason[410] = "Gone";
	reason[411] = "Length Required";
	reason[412] = "Precondition Failed";
	reason[413] = "Payload Too Large";
	reason[414] = "URI Too Long";
	reason[415] = "Unsupported Media Type";
	reason[416] = "Range Not Satisfiable";
	reason[417] = "Expectation Failed";
	reason[428] = "Precondition Required";
	reason[429] = "Too Many Requests";
	reason[431] = "Request Header Fields Too Large";
	reason[426] = "Upgrade Required";
	reason[500] = "Internal Server Error";
	reason[501] = "Not Implemented";
	reason[502] = "Bad Gateway";
	reason[503] = "Service Unavailable";
	reason[504] = "Gateway Timeout";
	reason[505] = "HTTP Version Not Supported";
	reason[511] = "Network Authentication Required";
	return (reason);
}

std::string ReasonPhrase::get(int statusCode)
{
	if (statusCode < 100 || statusCode >= 600)
		return ("");

	if (reason.find(statusCode) == reason.end())
		statusCode = (statusCode / 100) * 100; // e.g. 2xx -> 200

	return (reason[statusCode]);
}
