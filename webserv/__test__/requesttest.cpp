#include "../src/request/request.hpp"
#include "../src/Logger.hpp"
#include <iostream>
#include <string>
#include <unistd.h>

#define EXAMPLE1                                                                                   \
	"GET / HTTP/1.1\n\
Host : www.google.com "

#define EXAMPLE2                                                                                   \
	"CONNECT www.example.com:80 HTTP/1.1\nHost: www.example.com \nContent-Type: "                    \
	"plain/text\nContent-Length: 234\nContent-Length: 234\n\n"

#define EXAMPLE3                                                                                   \
	"POST /where?q=hello&name=hihi HTTP/1.1\n\
Host: www.webser.com"

void test(const char *buffer, ssize_t octetSize, ssize_t bufsize)
{
	Request req;
	char *ptr;

	ptr = (char *)malloc(bufsize * sizeof(char));
	for (size_t i = 0; i < octetSize; i += bufsize)
	{
		if (i + bufsize >= octetSize)
			bufsize = strlen(buffer + i);
		memcpy(ptr, &buffer[i], bufsize);
		std::cout << "not done... reading...\n";
		if (req.fillBuffer(ptr, bufsize) < 0)
		{
			std::cout << "error occured\n";
			return;
		}
	}
	std::cout << "reading and parsing finished\n";
	free(ptr);
}

int main(void)
{
	// test("CONNECT www.example.com:80 HTTP/1.1\nHost: www.example.com \nContent-Type: "
	// 		 "plain/text\nContent-Length: 234\nContent-Length: 234\n\n",
	// 		 16);
	Logger::init(Logger::LOGLEVEL_DEBUG, "/dev/stderr");
	test("POST / HTTP/1.1\n\
Host: localhost:8000\n\
User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux i686; rv:29.0) Gecko/20100101 Firefox/29.0\n\
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\n\
Accept-Language: en-US,en;q=0.5\n\
Accept-Encoding: gzip, deflate\n\
Cookie: __atuvc=34%7C7; permanent=0; _gitlab_session=226ad8a0be43681acf38c2fab9497240; __profilin=p%3Dt; request_method=GET\n\
Connection: keep-alive\n\
Content-Type: multipart/form-data; boundary=-----------------------------9051914041544843365972754266\n\
Content-Length: 555\n\
\n\
-----------------------------9051914041544843365972754266\r\n\
Content-Disposition: form-data; name=\"text\"\r\n\
\r\n\
text default\r\n\
-----------------------------9051914041544843365972754266\r\n\
Content-Disposition: form-data; name=\"file1\"; filename=\"a.txt\"\r\n\
Content-Type: text/plain\r\n\
\r\n\
Content \0of a.txt.\r\n\
\r\n\
-----------------------------9051914041544843365972754266\r\n\
Content-Disposition: form-data; name=\"file2\"; filename=\"a.html\"\r\n\
Content-Type: text/html\r\n\
\r\n\
<!DOCTYPE html><title>Content of a.html.</title>\r\n\
\r\n\
-----------------------------9051914041544843365972754266--",
			 1084, 1);
			 std::cout << ::strlen("CONNECT / HTTP/1.1\r\n\
Host: localhost\r\n\
Content-Type: text/plain \r\n\
Transfer-Encoding: chunked\r\n\
\r\n\
7\r\n\
Mozilla\r\n\
9\r\n\
Developer\r\n\
7\r\n\
Network\r\n\
0\r\n\
\r\n") << std::endl;
	test("CONNECT / HTTP/1.1\r\n\
Host: localhost\r\n\
Content-Type: text/plain \r\n\
Transfer-Encoding: chunked\r\n\
\r\n\
7\r\n\
Mozilla\r\n\
9\r\n\
Developer\r\n\
7\r\n\
Network\r\n\
0\r\n\
\r\n",
			 137, 1);
	
	system("leaks a.out");
}
