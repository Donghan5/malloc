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

HEADERS = $(PATH_INC)/functions.h $(PATH_INC)/struct.h \
		$(PATH_INC)/malloc.h $(PATH_INC)/define.h

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

$(PATH_OBJ)/%.o: %.c $(HEADERS)
	@mkdir -p $(@D)
	$(CC) -c -o $@ $(FLAGS_CC) $< -O0 -g -I $(PATH_INC)

clean:
	@rm -rf $(PATH_OBJ)
	@echo "Clean done"

fclean: clean
	@rm -f $(NAME) $(LIB_NAME) $(TEST_EXEC) $(EDGE_TEST_EXEC) $(M1_TEST_EXEC) $(TEST_RUNNER)
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

# EDGE CASE TESTING RULES
TEST_RUNNER = tests/test_runner
EDGE_TEST_EXEC = tests/edge_cases
EDGE_TEST_HEADERS = $(PATH_INC)/malloc.h $(PATH_INC)/struct.h
EDGE_TEST_HEADERS += $(PATH_INC)/functions.h $(PATH_INC)/define.h
EDGE_TEST_CPPFLAGS = -I$(PATH_INC) -Dmalloc=edge_malloc -Dfree=edge_free -Drealloc=edge_realloc
EDGE_TEST_CFLAGS = -std=gnu11 -Wall -Wextra -Werror -O0 -g -fno-builtin

.PHONY: test

test: $(EDGE_TEST_EXEC) $(TEST_RUNNER)
	./$(TEST_RUNNER) ./$(EDGE_TEST_EXEC)

$(EDGE_TEST_EXEC): tests/edge_cases.c tests/test_helpers.c tests/test_helpers.h $(SOURCES) $(EDGE_TEST_HEADERS) Makefile
	$(CC) $(EDGE_TEST_CPPFLAGS) $(EDGE_TEST_CFLAGS) tests/edge_cases.c tests/test_helpers.c $(SOURCES) -pthread -o $@

# M1 contract suite includes mandatory regressions and isolated build checks.
.PHONY: test-m1
M1_TEST_EXEC = tests/m1_cases

test-m1: $(M1_TEST_EXEC) $(TEST_RUNNER)
	./$(TEST_RUNNER) ./$(M1_TEST_EXEC) --m1

$(M1_TEST_EXEC): tests/edge_cases.c tests/m1_cases.c tests/m1_cases.h tests/test_helpers.c tests/test_helpers.h $(SOURCES) $(EDGE_TEST_HEADERS) Makefile
	$(CC) $(EDGE_TEST_CPPFLAGS) $(EDGE_TEST_CFLAGS) -DM1_TESTS tests/edge_cases.c tests/m1_cases.c tests/test_helpers.c $(SOURCES) -pthread -o $@


$(TEST_RUNNER): tests/run_tests.c Makefile
	$(CC) -std=gnu11 -Wall -Wextra -Werror -O0 -g $< -o $@
