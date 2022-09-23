.PHONY: all
all: webserv

.PHONY: webserv
webserv:
	c++ *.cpp **/*.cpp -g -o webserv -I.

.PHONY: clean
clean:
# rm *.o

.PHONY: fclean
fclean: clean
	rm webserv

.PHONY: re
re : fclean all