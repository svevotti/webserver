GNU = c++

#flags for debugging
# FLAGS = -Wall -Wextra -Werror -Wall -Wunused-function -fsanitize=address
# Normal flags
FLAGS = -Wall -Wextra -Werror -Wall -Wunused-function -ggdb3 -std=c++98
# C_98 = -std=c++98
OBJ_DIR = obj
CPP_FILES = ServerSockets.cpp main.cpp HttpResponse.cpp \
			WebServer.cpp HttpRequest.cpp Logger.cpp \
			ClientHandler.cpp Config.cpp InfoServer.cpp \
			HttpException.cpp CGI.cpp CGIHandler.cpp
CPP_OBJ = $(OBJ_DIR)/ServerSockets.o $(OBJ_DIR)/main.o \
          $(OBJ_DIR)/HttpResponse.o \
          $(OBJ_DIR)/Logger.o \
		  $(OBJ_DIR)/WebServer.o $(OBJ_DIR)/HttpRequest.o \
		  $(OBJ_DIR)/ClientHandler.o  $(OBJ_DIR)/Config.o \
		  $(OBJ_DIR)/InfoServer.o $(OBJ_DIR)/HttpException.o \
		  $(OBJ_DIR)/CGI.o \
		  $(OBJ_DIR)/CGIHandler.o

NAME = webserver

all: $(NAME)

$(NAME): $(OBJ_DIR) $(CPP_OBJ)
	$(GNU) $(CPP_OBJ) -o $(NAME)
# when compiling with the -fsanitize flag replace the line above with this one
# $(GNU) $(CPP_OBJ) -fsanitize=address -o $(NAME) 
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: %.cpp
	$(GNU) $(FLAGS) $(C_98) -c $^ -o $@

clean:
	rm -f $(CPP_OBJ)
fclean: clean
	rm -f $(NAME)
re: fclean all
.PHONY: clean fclean re all
