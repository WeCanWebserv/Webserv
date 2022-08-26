#include "../src/request/request.hpp"
#include "../src/request/request_parser.hpp"
#include <iostream>
#include <string>

#define EXAMPLE1                                                                                   \
	"GET / HTTP/1.1\n\
Host : www.google.com "

#define EXAMPLE2 "CONNECT www.example.com:80 HTTP/1.1"
#define EXAMPLE2_1 "\nHost: www.example.com "

#define EXAMPLE3                                                                                   \
	"POST /where?q=hello&name=hihi HTTP/1.1\n\
Host: www.webser.com"

int main(void)
{
	Request req;

	std::cout << EXAMPLE2 << std::endl;

	req.fillBuffer(EXAMPLE2, strlen(EXAMPLE2));
	std::cout << "not done\n";
	req.fillBuffer(EXAMPLE2_1, strlen(EXAMPLE2_1));
}