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

fclean: clean test-fclean
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

# Functional test suites. Only unit fixtures rename allocator entry points.
TEST_BIN = test/bin
TEST_OBJ = test/obj
UNIT_FLAGS = -std=gnu11 -Wall -Wextra -Werror -O0 -g -fno-builtin
UNIT_INCLUDES = -Iinc -Itest/helpers -Itest/ui
UNIT_RENAMES = -Dmalloc=edge_malloc -Dfree=edge_free -Drealloc=edge_realloc
TEST_COMMON = test/helpers/test_helpers.c test/helpers/bonus_helpers.c test/ui/test_ui.c
TEST_HEADERS = $(HEADERS) $(wildcard test/helpers/*.h test/ui/*.h)
MANDATORY_BINS = $(patsubst test/%.c,$(TEST_BIN)/%,$(wildcard test/mandatory/*.c))
BONUS_BINS = $(patsubst test/%.c,$(TEST_BIN)/%,$(wildcard test/bonus/*.c))
INTEGRATION_BINS = $(addprefix $(TEST_BIN)/integration/,build_contracts preload workload)
ALL_TEST_EXEC = $(TEST_BIN)/all_cases
TEST_BINS = $(MANDATORY_BINS) $(BONUS_BINS) $(INTEGRATION_BINS) $(ALL_TEST_EXEC)
CORRECTION_SOURCES = $(sort $(wildcard test/correction/test*.c))
CORRECTION_BINS = $(patsubst test/%.c,$(TEST_BIN)/%,$(CORRECTION_SOURCES))
.PHONY: test test-all test-build test-mandatory test-bonus test-integration test-clean test-fclean test-correction test-correction-build

test test-all: test-build
	./$(ALL_TEST_EXEC)

test-build: $(TEST_BINS) $(NAME)

test-mandatory: $(MANDATORY_BINS) $(ALL_TEST_EXEC)
	./$(ALL_TEST_EXEC) mandatory

test-bonus: $(BONUS_BINS) $(ALL_TEST_EXEC)
	./$(ALL_TEST_EXEC) bonus

test-integration: $(INTEGRATION_BINS) $(ALL_TEST_EXEC) $(NAME)
	./$(ALL_TEST_EXEC) integration

$(MANDATORY_BINS) $(BONUS_BINS): $(TEST_BIN)/%: test/%.c $(TEST_COMMON) $(SOURCES) $(TEST_HEADERS) Makefile
	@mkdir -p $(@D)
	$(CC) $(UNIT_FLAGS) $(UNIT_INCLUDES) $(UNIT_RENAMES) $< $(TEST_COMMON) $(SOURCES) -pthread -o $@

$(ALL_TEST_EXEC): test/all_cases/all_cases.c Makefile
	@mkdir -p $(@D)
	$(CC) $(UNIT_FLAGS) $< -o $@

$(TEST_BIN)/integration/build_contracts: test/integration/build_contracts.c test/ui/test_ui.c $(TEST_HEADERS) Makefile
	@mkdir -p $(@D)
	$(CC) $(UNIT_FLAGS) $(UNIT_INCLUDES) $< test/ui/test_ui.c -o $@

$(TEST_BIN)/integration/preload: test/integration/preload.c Makefile
	@mkdir -p $(@D)
	$(CC) $(UNIT_FLAGS) $< -o $@

$(TEST_BIN)/integration/workload: test/integration/workload.c Makefile
	@mkdir -p $(@D)
	$(CC) $(UNIT_FLAGS) -fPIE -pie $< -ldl -o $@

$(CORRECTION_BINS): $(TEST_BIN)/correction/%: test/correction/%.c $(NAME) $(HEADERS) Makefile
	@mkdir -p $(@D)
	$(CC) -std=gnu11 -Wall -Wextra -O0 -g -fno-builtin -Iinc $< ./$(NAME) -pthread -Wl,-rpath,'$$ORIGIN/../../..' -o $@

test-correction-build: $(CORRECTION_BINS)
test-correction: test-correction-build
	@failed=0; for binary in $(CORRECTION_BINS); do "./$$binary" || failed=1; done; exit $$failed

test-clean:
	@rm -rf $(TEST_OBJ)
test-fclean: test-clean
	@rm -rf $(TEST_BIN)
	@rm -f $(TEST_EXEC)
