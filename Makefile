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
BUILD_CONTRACTS = $(TEST_BIN)/integration/build_contracts
FAULT_PROBE = $(TEST_BIN)/integration/fault_probe
EVAL_BINS = $(addprefix $(TEST_BIN)/eval/,test0 test1 test2)
CORRECTION_BINS = $(patsubst test/correction/%.c,$(TEST_BIN)/correction/%,$(wildcard test/correction/test*.c))
ALL_TEST_EXEC = $(TEST_BIN)/all_cases
TEST_BINS = $(MANDATORY_BINS) $(BONUS_BINS) $(BUILD_CONTRACTS) $(FAULT_PROBE) $(EVAL_BINS) $(ALL_TEST_EXEC)
REPEATS ?= 1
.PHONY: test test-build test-mandatory test-bonus test-eval test-free-quality test_correction test-correction test-diagnostic-build test-clean test-fclean

test: test-build
	./$(ALL_TEST_EXEC)

test-build: $(TEST_BINS) $(NAME)

test-mandatory: $(MANDATORY_BINS) $(ALL_TEST_EXEC)
	./$(ALL_TEST_EXEC) mandatory

test-bonus: $(BONUS_BINS) $(ALL_TEST_EXEC)
	./$(ALL_TEST_EXEC) bonus

test-eval: $(NAME) $(EVAL_BINS) $(FAULT_PROBE)
	sh test/integration/run.sh eval $(REPEATS)

# correction 전체 기준을 함께 확인하며 free 품질 실패도 종료 코드로 전파한다.
test-free-quality: test-eval

# 기능 실행용 직접 링크 검사. 페이지 품질 평가는 test-free-quality를 사용한다.
test_correction: $(CORRECTION_BINS)
	@ulimit -c 0; failed=0; \
	for binary in $(CORRECTION_BINS); do \
		echo "실행: $$binary"; \
		if timeout --signal=TERM --kill-after=2s 15s ./$$binary; then \
			echo "정상 종료: $$binary"; \
		else \
			status=$$?; echo "실행 실패: $$binary (종료 코드 $$status)"; failed=1; \
		fi; \
	done; exit $$failed

test-correction: test_correction

$(CORRECTION_BINS): $(TEST_BIN)/correction/%: test/correction/%.c $(HEADERS) $(NAME) Makefile
	@mkdir -p $(@D)
	$(CC) -std=gnu11 -Wall -Wextra -O0 -g -fno-builtin $< -Iinc $(abspath $(NAME)) -pthread -o $@

test-diagnostic-build: $(NAME) $(FAULT_PROBE)

# correction 원본은 libc만 링크하며 실제 주입은 실행 시 수행한다.
$(EVAL_BINS): $(TEST_BIN)/eval/%: test/correction/%.c $(HEADERS) Makefile
	@mkdir -p $(@D)
	$(CC) -std=gnu11 -Wall -Wextra -O0 -g -fno-builtin $< -o $@

$(FAULT_PROBE): test/integration/fault_probe.c Makefile
	@mkdir -p $(@D)
	$(CC) $(UNIT_FLAGS) -fPIE -pie $< -ldl -o $@

$(MANDATORY_BINS) $(BONUS_BINS): $(TEST_BIN)/%: test/%.c $(TEST_COMMON) $(SOURCES) $(TEST_HEADERS) Makefile
	@mkdir -p $(@D)
	$(CC) $(UNIT_FLAGS) $(UNIT_INCLUDES) $(UNIT_RENAMES) $< $(TEST_COMMON) $(SOURCES) -pthread -o $@

$(ALL_TEST_EXEC): test/all_cases/all_cases.c Makefile
	@mkdir -p $(@D)
	$(CC) $(UNIT_FLAGS) $< -o $@

$(TEST_BIN)/integration/build_contracts: test/integration/build_contracts.c test/ui/test_ui.c $(TEST_HEADERS) Makefile
	@mkdir -p $(@D)
	$(CC) $(UNIT_FLAGS) $(UNIT_INCLUDES) $< test/ui/test_ui.c -o $@

test-clean:
	@rm -rf $(TEST_OBJ)
test-fclean: test-clean
	@rm -rf $(TEST_BIN)
	@rm -f $(TEST_EXEC)
