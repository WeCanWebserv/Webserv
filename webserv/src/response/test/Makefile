NAME := test-response

D_SRC := ..
D_OBJ := .
D_INC := ..

SRC := main.cpp
# SRC += Response.cpp
SRC += UriParser.cpp
SRC += ReasonPhrase.cpp
SRC += MediaType.cpp

OBJ := $(addprefix $(D_OBJ)/, $(SRC:.cpp=.o))

DEP := $(OBJ:.o=.d)

INC := -I$(D_INC)

CXX := c++
CXXFLAGS := -std=c++98 -Wall -Werror -Wextra
CXXFLAGS += $(INC)

.PHONY: test
test: all
	./$(NAME)

.PHONY: all
all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

$(D_OBJ)/%.o: $(D_SRC)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(D_OBJ)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(RM) $(OBJ)

.PHONY: fclean
fclean: clean
	$(RM) $(NAME)

.PHONY: re
re: fclean all

-include $(DEP)
