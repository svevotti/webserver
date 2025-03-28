GNU = c++
FLAGS = -Wall -Wextra #-Werror
C_98 = -std=c++98
OBJ_DIR = obj
CPP_FILES = ServerSockets.cpp main.cpp HttpResponse.cpp \
			WebServer.cpp HttpRequest.cpp Logger.cpp \
			ClientHandler.cpp Config.cpp Server.cpp \
			HttpException.cpp
CPP_OBJ = $(OBJ_DIR)/ServerSockets.o $(OBJ_DIR)/main.o \
          $(OBJ_DIR)/HttpResponse.o \
          $(OBJ_DIR)/Logger.o \
		  $(OBJ_DIR)/WebServer.o $(OBJ_DIR)/HttpRequest.o \
		  $(OBJ_DIR)/ClientHandler.o  $(OBJ_DIR)/Config.o \
		  $(OBJ_DIR)/Server.o $(OBJ_DIR)/HttpException.o

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
