GNU = c++
FLAGS = -Wall -Wextra -Werror
C_98 = -std=c++98
CPP_FILES =  main.cpp Config.cpp InfoServer.cpp
CPP_OBJ = $(CPP_FILES:.cpp=.o)
NAME = server

all: $(NAME)

$(NAME): $(CPP_OBJ)
	$(GNU) $(CPP_OBJ) -o $(NAME)

%.o: %.cpp
	$(GNU) $(FLAGS) $(C_98) -c $^

clean:
	rm -f $(CPP_OBJ)
fclean: clean
	rm -f $(NAME)
re: fclean all
.PHONY: clean fclean re all
