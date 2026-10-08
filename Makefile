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
	@rm -f $(NAME) $(LIB_NAME) $(TEST_EXEC) $(TEST_BINS)
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

# Structured test sources; generated executables live only in test/bin.
TEST_ROOT = test
TEST_BIN = $(TEST_ROOT)/bin
TEST_RUNNER = $(TEST_BIN)/test_runner
EDGE_TEST_EXEC = $(TEST_BIN)/edge_cases
M1_TEST_EXEC = $(TEST_BIN)/m1_cases
M2_TEST_EXEC = $(TEST_BIN)/m2_cases
BONUS_EXEC = $(addprefix $(TEST_BIN)/,m3_cases m4_cases m5_cases)
ALL_TEST_EXEC = $(TEST_BIN)/all_cases
TEST_BINS = $(EDGE_TEST_EXEC) $(M1_TEST_EXEC) $(M2_TEST_EXEC) $(BONUS_EXEC) $(TEST_RUNNER) $(ALL_TEST_EXEC)
TEST_INCLUDES = -I$(PATH_INC) -I$(TEST_ROOT)/helpers -I$(TEST_ROOT)/ui \
                -I$(TEST_ROOT)/milestones -I$(TEST_ROOT)/edge_cases -I$(TEST_ROOT)/all_cases
EDGE_TEST_CPPFLAGS = $(TEST_INCLUDES) -Dmalloc=edge_malloc -Dfree=edge_free -Drealloc=edge_realloc
EDGE_TEST_CFLAGS = -std=gnu11 -Wall -Wextra -Werror -O0 -g -fno-builtin
TEST_COMMON = $(TEST_ROOT)/helpers/test_helpers.c $(TEST_ROOT)/helpers/bonus_helpers.c $(TEST_ROOT)/ui/test_ui.c
TEST_HEADERS = $(HEADERS) $(wildcard $(TEST_ROOT)/helpers/*.h $(TEST_ROOT)/ui/*.h $(TEST_ROOT)/milestones/*.h $(TEST_ROOT)/edge_cases/*.h $(TEST_ROOT)/all_cases/*.h)
EDGE_SOURCE = $(TEST_ROOT)/edge_cases/edge_cases.c
EDGE_BONUS = $(TEST_ROOT)/edge_cases/bonus_edge_cases.c

.PHONY: test test-edge test-m1 test-m2 test-m3 test-m4 test-m5 test-all test-build

test test-all: test-build
	./$(ALL_TEST_EXEC)

test-build: $(TEST_BINS)

test-edge: $(EDGE_TEST_EXEC) $(ALL_TEST_EXEC)
	./$(ALL_TEST_EXEC) edge

test-m1: $(M1_TEST_EXEC) $(TEST_RUNNER)
	./$(TEST_RUNNER) ./$(M1_TEST_EXEC) --m1

test-m2: $(M2_TEST_EXEC) $(TEST_RUNNER)
	./$(TEST_RUNNER) ./$(M2_TEST_EXEC) --m2

test-m3: $(TEST_BIN)/m3_cases $(ALL_TEST_EXEC)
	./$(ALL_TEST_EXEC) 3

test-m4: $(TEST_BIN)/m4_cases $(ALL_TEST_EXEC)
	./$(ALL_TEST_EXEC) 4

test-m5: $(TEST_BIN)/m5_cases $(ALL_TEST_EXEC)
	./$(ALL_TEST_EXEC) 5

$(EDGE_TEST_EXEC): $(EDGE_SOURCE) $(EDGE_BONUS) $(TEST_COMMON) $(SOURCES) $(TEST_HEADERS) Makefile
	@mkdir -p $(@D)
	$(CC) $(EDGE_TEST_CPPFLAGS) $(EDGE_TEST_CFLAGS) $(EDGE_SOURCE) $(EDGE_BONUS) $(TEST_COMMON) $(SOURCES) -pthread -o $@

$(M1_TEST_EXEC): $(EDGE_SOURCE) $(TEST_ROOT)/milestones/m1_cases.c $(TEST_COMMON) $(SOURCES) $(TEST_HEADERS) Makefile
	@mkdir -p $(@D)
	$(CC) $(EDGE_TEST_CPPFLAGS) $(EDGE_TEST_CFLAGS) -DM1_TESTS $(EDGE_SOURCE) $(TEST_ROOT)/milestones/m1_cases.c $(TEST_COMMON) $(SOURCES) -pthread -o $@

$(M2_TEST_EXEC) $(BONUS_EXEC): $(TEST_BIN)/%: $(TEST_ROOT)/milestones/%.c $(TEST_COMMON) $(SOURCES) $(TEST_HEADERS) Makefile
	@mkdir -p $(@D)
	$(CC) $(EDGE_TEST_CPPFLAGS) $(EDGE_TEST_CFLAGS) $< $(TEST_COMMON) $(SOURCES) -pthread -o $@

$(ALL_TEST_EXEC): $(TEST_ROOT)/all_cases/all_cases.c $(TEST_COMMON) $(SOURCES) $(TEST_HEADERS) Makefile
	@mkdir -p $(@D)
	$(CC) $(EDGE_TEST_CPPFLAGS) $(EDGE_TEST_CFLAGS) $< $(TEST_COMMON) $(SOURCES) -pthread -o $@

$(TEST_RUNNER): $(TEST_ROOT)/run_tests.c $(TEST_ROOT)/ui/test_ui.c $(TEST_HEADERS) Makefile
	@mkdir -p $(@D)
	$(CC) $(TEST_INCLUDES) $(EDGE_TEST_CFLAGS) $< $(TEST_ROOT)/ui/test_ui.c -o $@
