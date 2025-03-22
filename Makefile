GNU = c++
FLAGS = -Wall -Wextra #-Werror
C_98 = -std=c++98
OBJ_DIR = obj
CPP_FILES = ServerSockets.cpp main.cpp HttpResponse.cpp InfoServer.cpp \
			Webserver.cpp HttpRequest.cpp Logger.cpp \
			ClientHandler.cpp
CPP_OBJ = $(OBJ_DIR)/ServerSockets.o $(OBJ_DIR)/main.o \
          $(OBJ_DIR)/HttpResponse.o $(OBJ_DIR)/InfoServer.o \
          $(OBJ_DIR)/Logger.o \
		  $(OBJ_DIR)/Webserver.o $(OBJ_DIR)/HttpRequest.o \
		   $(OBJ_DIR)/ClientHandler.o

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
