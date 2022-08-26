#include "../src/request/request.hpp"
#include <iostream>
#include <string>

#define EXAMPLE1                                                                                   \
	"GET / HTTP/1.1\n\
Host : www.google.com "

#define EXAMPLE2 "CONNECT www.example.com:80 HTTP/1.1"
#define EXAMPLE2_1 "\nHost: www.example.com \n"
#define EXAMPLE2_2 "Content-Type: "
#define EXAMPLE2_3 " plain/text"
#define EXAMPLE2_4 "\nContent-Length: 234\n"
#define EXAMPLE2_5 "Content-Length: 234\n"
#define EXAMPLE2_6 "\n"

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
	std::cout << "not done\n";
	req.fillBuffer(EXAMPLE2_2, strlen(EXAMPLE2_2));
	std::cout << "not done\n";
	req.fillBuffer(EXAMPLE2_3, strlen(EXAMPLE2_3));
	std::cout << "not done\n";
	req.fillBuffer(EXAMPLE2_4, strlen(EXAMPLE2_4));
	std::cout << "not done\n";
	req.fillBuffer(EXAMPLE2_5, strlen(EXAMPLE2_5));
	std::cout << "not done\n";
	req.fillBuffer(EXAMPLE2_6, strlen(EXAMPLE2_6));
	std::cout << "done\n";
}