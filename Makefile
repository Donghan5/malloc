ifeq ($(HOSTTYPE),)
	HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

# **************************************************************************** #
# FILES             														   #
# **************************************************************************** #

PATH_INC = inc
PATH_LIB = lib
PATH_OBJ = obj
PATH_SRC = src

SOURCES += src/block/block.c src/block/init_block.c
SOURCES += src/heap/get_heap.c src/heap/heap.c src/heap/helper_heap.c
SOURCES += src/tools/show_alloc_mem.c src/tools/tools.c src/tools/pointer.c
SOURCES += src/free.c src/malloc.c src/realloc.c

OBJECTS = $(SOURCES:%.c=$(PATH_OBJ)/%.o)

# **************************************************************************** #
# VARIABLES         														   #
# **************************************************************************** #

NAME = libft_malloc_$(HOSTTYPE).so
LIB_NAME = libft_malloc.so

CC = gcc

FLAGS_CC = -Wall -Wextra -Werror -fPIC
FLAGS_LIB = -shared

# **************************************************************************** #
# COMMANDS  		    													   #
# **************************************************************************** #

.PHONY: all clean fclean re setup

all: setup $(NAME)

$(NAME): $(OBJECTS)
	$(CC) $(FLAGS_LIB) -o $@ $(OBJECTS)
	@rm -f $(LIB_NAME)
	ln -s $(NAME) $(LIB_NAME)
	@echo "Make done"

$(PATH_OBJ)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) -c -o $@ $(FLAGS_CC) $^ -O0 -g -I $(PATH_INC)

clean:
	@rm -rf $(PATH_OBJ)
	@echo "Clean done"

fclean: clean
	@rm -f $(NAME) $(LIB_NAME) $(TEST_EXEC)
	@echo "Fclean done"

setup:
	./setup.sh

re: fclean $(NAME)

TEST_EXEC = test_malloc

# TESTING RULES

.PHONY: run $(TEST_EXEC)

run: all $(TEST_EXEC)
	./$(TEST_EXEC)
	@echo "--- Test finished ---"

$(TEST_EXEC): main.c $(NAME)
	$(CC) main.c -o $(TEST_EXEC) -I $(PATH_INC) ./$(NAME) -lpthread

valgrind: all $(TEST_EXEC)
	valgrind --soname-synonyms=somalloc=libft_malloc_x86_64_Linux.so ./$(TEST_EXEC)
	@echo "--- Valgrind finished ---"

debug_mode: all $(TEST_EXEC)
	MALLOC_DEBUG=1 ./$(TEST_EXEC)

scribble_mode: all $(TEST_EXEC)
	MALLOC_SCRIBBLE=1 ./$(TEST_EXEC)