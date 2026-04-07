CC       := gcc
CFLAGS   := -std=c11 -Wall -Wextra -Wpedantic -Wshadow -g -O1 \
            -fsanitize=undefined \
            -I../lib -Iunity

UNITY_VERSION := v2.6.1
UNITY_URL     := https://github.com/ThrowTheSwitch/Unity/archive/refs/tags/$(UNITY_VERSION).tar.gz
UNITY_SHA256  := b41a66d45a6b99758fb3202ace6178177014d52fc524bf1f72687d93e9867292
UNITY_SRC     := unity/unity.c
UNITY_HEADERS := unity/unity.h unity/unity_internals.h

LIB_SRCS := $(wildcard ../lib/*.c)

TEST_SRCS := $(wildcard test_*.c)
TEST_BINS := $(TEST_SRCS:test_%.c=build/test_%)

BUILD := build

BEAR := $(shell command -v bear 2>/dev/null)

.PHONY: all test valgrind clean compile_commands

all: $(TEST_BINS) compile_commands

$(BUILD):
	@mkdir -p $(BUILD)

# Fetch Unity, verify checksum, extract only the src/ files we need.
$(UNITY_SRC) $(UNITY_HEADERS):
	@mkdir -p unity
	@echo "Fetching Unity $(UNITY_VERSION)..."
	@curl -sSfL $(UNITY_URL) -o unity/unity.tar.gz
	@echo "$(UNITY_SHA256)  unity/unity.tar.gz" | sha256sum -c -
	@tar -xzf unity/unity.tar.gz -C unity --strip-components=2 --wildcards '*/src/unity.c' '*/src/unity.h' '*/src/unity_internals.h'
	@rm unity/unity.tar.gz

build/test_%: test_%.c $(LIB_SRCS) $(UNITY_SRC) $(UNITY_HEADERS) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $< $(LIB_SRCS) $(UNITY_SRC)

test: $(TEST_BINS)
	@fail=0; \
	for t in $(TEST_BINS); do \
		echo "==> $$t"; \
		$$t || fail=1; \
	done; \
	exit $$fail

valgrind: $(TEST_BINS)
	@fail=0; \
	for t in $(TEST_BINS); do \
		echo "==> valgrind $$t"; \
		valgrind --error-exitcode=1 --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all $$t || fail=1; \
	done; \
	exit $$fail

compile_commands: $(TEST_BINS)
ifdef BEAR
	@bear --output ../compile_commands.json -- $(MAKE) --no-print-directory -B $(TEST_BINS)
endif

clean:
	@rm -rf $(BUILD) unity
