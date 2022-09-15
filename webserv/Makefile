.PHONY				:		all clean fclean re

NAME					=		webserv

CXX						= 	c++
CXXFLAGS			=		-Wall -Wextra -Werror -std=c++98
INC						=		-I./src/

DEFAULT_DIR		= 	./src
REQUEST_SRCS	=		$(addprefix $(DEFAULT_DIR)/request/, \
											request_manager.cpp \
											request_parser.cpp \
											request.cpp\
											)

RESPONSE_SRCS	=		$(addprefix $(DEFAULT_DIR)/response/, \
											Response.cpp \
											Cgi.cpp \
											MediaType.cpp \
											ReasonPhrase.cpp \
											UriParser.cpp \
											)

SRCS					=		$(REQUEST_SRCS) \
									$(RESPONSE_SRCS) \
									$(addprefix $(DEFAULT_DIR)/, \
											ConfigParser.cpp \
											Logger.cpp \
											ServerManager.cpp \
											libft.cpp \
											main.cpp \
											)

OBJS					=		$(SRCS:.cpp=.o)

DEPS					=		$(OBJS:.o=.d)

all			: 	$(NAME)

$(NAME)	:		$(OBJS)
	$(CXX) $(CXXFLAGS) $(INC) -o $@ $^

%.o			:		%.cpp
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@ -MD

clean		:
	rm -rf $(OBJS) $(DEPS)

fclean	:	clean
	rm -rf $(NAME)

re			:
	make fclean
	make all

-include $(DEPS)