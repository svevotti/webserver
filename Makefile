GNU = c++
FLAGS = -Wall -Wextra #-Werror
C_98 = -std=c++98
OBJ_DIR = obj
CPP_FILES = ServerSockets.cpp main.cpp ClientRequest.cpp ServerResponse.cpp InfoServer.cpp \
			StringManipulations.cpp PrintingFunctions.cpp \
			Webserver.cpp HttpRequest.cpp
CPP_OBJ = $(OBJ_DIR)/ServerSockets.o $(OBJ_DIR)/main.o $(OBJ_DIR)/ClientRequest.o \
          $(OBJ_DIR)/ServerResponse.o $(OBJ_DIR)/InfoServer.o \
          $(OBJ_DIR)/StringManipulations.o $(OBJ_DIR)/PrintingFunctions.o \
		  $(OBJ_DIR)/Webserver.o $(OBJ_DIR)/HttpRequest.o

NAME = server

all: $(NAME)

$(NAME): $(CPP_OBJ)
	$(GNU) $(CPP_OBJ) -o $(NAME)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(OBJ_DIR)
	$(GNU) $(FLAGS) $(C_98) -c $< -o $@

clean:
	rm -f $(OBJ_DIR)/*.o

fclean: clean
	rm -f $(NAME)
	rm -rf $(OBJ_DIR)
re: fclean all
.PHONY: clean fclean re all