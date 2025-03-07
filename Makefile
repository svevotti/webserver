GNU = c++
FLAGS = -Wall -Wextra #-Werror
C_98 = #-std=c++98
OBJ_DIR = obj
CPP_FILES = ServerSockets.cpp main.cpp ClientRequest.cpp ServerResponse.cpp InfoServer.cpp \
			StringManipulations.cpp Webserver.cpp HttpRequest.cpp Logger.cpp
CPP_OBJ = $(OBJ_DIR)/ServerSockets.o $(OBJ_DIR)/main.o $(OBJ_DIR)/ClientRequest.o \
          $(OBJ_DIR)/ServerResponse.o $(OBJ_DIR)/InfoServer.o \
          $(OBJ_DIR)/StringManipulations.o $(OBJ_DIR)/Logger.o \
		  $(OBJ_DIR)/Webserver.o $(OBJ_DIR)/HttpRequest.o

NAME = webserver

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
