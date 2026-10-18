ifeq ($(strip $(HOSTTYPE)),)
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
SOURCES += src/free.c src/malloc.c src/realloc.c src/calloc.c

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
ALLOCATOR_FLAGS = $(FLAGS_CC) -O0 -g
ALLOCATOR_INCLUDES = -I $(PATH_INC)
FLAGS_LIB = -shared

# **************************************************************************** #
# COMMANDS  		    													   #
# **************************************************************************** #

.PHONY: all clean fclean re

all: $(LIB_NAME)

$(NAME): $(OBJECTS)
	$(CC) $(FLAGS_LIB) -o $@ $(OBJECTS)
	@rm -f $(LIB_NAME)
	ln -s $(NAME) $(LIB_NAME)
	@echo "Make done"

$(LIB_NAME): $(NAME)
	ln -sf $(NAME) $@

$(PATH_OBJ)/%.o: %.c $(HEADERS) Makefile
	@mkdir -p $(@D)
	$(CC) -c -o $@ $(ALLOCATOR_FLAGS) $(ALLOCATOR_INCLUDES) $<

clean:
	@rm -rf $(PATH_OBJ)
	@echo "Clean done"

fclean: clean
	@rm -f $(NAME) $(LIB_NAME)
	@echo "Fclean done"

re:
	$(MAKE) fclean
	$(MAKE) all

# Linux tests: build and run only in isolated test workspaces.
TEST_TARGETS = test0 test1 test2 test3 test4 test5 test-edge test-bonus
.PHONY: $(TEST_TARGETS) test test-clean test-fclean check help
$(TEST_TARGETS):
	@bash test/eval/runner.sh $@

test:
	@sh test/eval/run.sh

test-clean:
	@rm -rf test/obj
	@echo "Test clean done"

test-fclean: test-clean
	@rm -rf test/bin test/evidence test/evaluation-*
	@rm -f test/core test/core.*
	@echo "Test fclean done"

check:
	@bash test/eval/check_runner.sh

help:
	@printf '%s\n' 'Build:' '  make / clean / fclean / re — allocator library' '' 'Mandatory:' \
	  '  test0      baseline: system/user malloc raw values' \
	  '  test1      allocation overhead / score interpretation; single-zone capacity and page size' \
	  '  test2      free function U2<U1 and quality U2-U0<=10' \
	  '  test3      realloc data preservation' \
	  '  test4      show_alloc_mem addresses, ranges and total' \
	  '  test5      alignment (ABI check; attachment coverage reported)' \
	  '  test-edge  O01-O04 additional overflow checks (outside official score)' \
	  '' 'Bonus:' '  test-bonus B01-B04 and C01-C08 calloc bonus checks' \
	  '' 'Test utilities:' \
	  '  test       run the complete evaluation once' \
	  '  test-clean remove test object files' \
	  '  test-fclean remove test objects, binaries, evaluation results and core dumps' \
	  '  check      run the shell runner fixtures only'
