# Makefile — Parking Garage Backend
#
# Targets:
#   make          — Build the 'parking' executable
#   make test     — Build and run the unit test suite
#   make clean    — Remove all build artifacts
#
# Compiler flags:
#   -Wall -Wextra  — Enable most warnings (catches common bugs)
#   -g             — Include debug symbols (useful during development)
#   -Iinclude      — Add include/ to the header search path

CC      = gcc
CFLAGS  = -Wall -Wextra -g -Iinclude
SRC_DIR = src
OBJ_DIR = obj
TEST_DIR = tests

# Source files for the main executable (everything except test files)
SRCS = $(SRC_DIR)/circular_queue.c \
       $(SRC_DIR)/priority_queue.c \
       $(SRC_DIR)/stack.c \
       $(SRC_DIR)/linked_list.c \
       $(SRC_DIR)/hash_table.c \
       $(SRC_DIR)/billing.c \
       $(SRC_DIR)/json_helpers.c \
       $(SRC_DIR)/garage.c \
       $(SRC_DIR)/main.c

# Source files for tests (data structures + billing, no main.c/garage.c/json)
TEST_SRCS = $(SRC_DIR)/circular_queue.c \
            $(SRC_DIR)/priority_queue.c \
            $(SRC_DIR)/stack.c \
            $(SRC_DIR)/linked_list.c \
            $(SRC_DIR)/hash_table.c \
            $(SRC_DIR)/billing.c \
            $(TEST_DIR)/test_all.c

# Default target: build the parking executable
all: parking

parking: $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

# Test target: build and immediately run tests
test: test_all
	./test_all

test_all: $(TEST_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

# Clean up build artifacts and state files
clean:
	rm -f parking test_all garage.dat
	rm -rf $(OBJ_DIR)

.PHONY: all test clean
