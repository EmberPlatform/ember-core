CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I../../../include -DEMBER_VERSION_STRING=\"2.0.3\"
DEBUG_FLAGS = -g -O0 -DDEBUG
RELEASE_FLAGS = -O2 -DNDEBUG

# Default to release build
BUILD_TYPE ?= release
ifeq ($(BUILD_TYPE),debug)
    CFLAGS += $(DEBUG_FLAGS)
else
    CFLAGS += $(RELEASE_FLAGS)
endif

BUILDDIR = ../../../build/testing
LIBDIR = ../../../build

# Testing framework sources
TESTING_SOURCES = testing_framework.c test_collections.c test_regex.c test_runner_main.c
TESTING_OBJECTS = $(TESTING_SOURCES:%.c=$(BUILDDIR)/%.o)

# Test executable
TEST_RUNNER = $(BUILDDIR)/ember_test_runner

# Main library
EMBER_LIB = $(LIBDIR)/libember.a

.PHONY: all clean test debug release help

all: $(TEST_RUNNER)

# Create build directory
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# Build test runner
$(TEST_RUNNER): $(TESTING_OBJECTS) $(EMBER_LIB) | $(BUILDDIR)
	$(CC) $(CFLAGS) $(TESTING_OBJECTS) -L$(LIBDIR) -lember -lm -lpthread -lssl -lcrypto -o $@

# Build testing framework objects
$(BUILDDIR)/%.o: %.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Ensure main library is built
$(EMBER_LIB):
	@echo "Building main Ember library..."
	@cd ../../.. && $(MAKE) all

# Run tests
test: $(TEST_RUNNER)
	@echo "Running Ember test suite..."
	@$(TEST_RUNNER)

# Build variants
debug:
	$(MAKE) BUILD_TYPE=debug

release:
	$(MAKE) BUILD_TYPE=release

# Run specific test suites
test-core: $(TEST_RUNNER)
	@echo "Running core tests only..."
	@$(TEST_RUNNER) --suite core

test-collections: $(TEST_RUNNER)
	@echo "Running collections tests only..."
	@$(TEST_RUNNER) --suite collections

test-regex: $(TEST_RUNNER)
	@echo "Running regex tests only..."
	@$(TEST_RUNNER) --suite regex

# Performance testing
benchmark: $(TEST_RUNNER)
	@echo "Running performance benchmarks..."
	@$(TEST_RUNNER) --benchmark

# Coverage testing (requires gcov)
coverage:
	$(MAKE) BUILD_TYPE=debug CFLAGS+="-fprofile-arcs -ftest-coverage"
	$(MAKE) test
	@echo "Generating coverage report..."
	@gcov $(TESTING_SOURCES)

# Memory testing with valgrind
memtest: $(TEST_RUNNER)
	@echo "Running memory tests with valgrind..."
	@valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all $(TEST_RUNNER)

# Cleanup
clean:
	rm -rf $(BUILDDIR)
	rm -f *.gcov *.gcda *.gcno

# Help
help:
	@echo "Ember Testing Framework Build System"
	@echo "===================================="
	@echo ""
	@echo "Main Targets:"
	@echo "  all              - Build test runner"
	@echo "  test             - Run complete test suite"
	@echo "  clean            - Remove build files"
	@echo ""
	@echo "Test Targets:"
	@echo "  test-core        - Run core functionality tests"
	@echo "  test-collections - Run collections framework tests"
	@echo "  test-regex       - Run regex functionality tests"
	@echo ""
	@echo "Build Variants:"
	@echo "  debug            - Build with debug symbols"
	@echo "  release          - Build optimized version (default)"
	@echo ""
	@echo "Advanced Targets:"
	@echo "  benchmark        - Run performance benchmarks"
	@echo "  coverage         - Generate code coverage report"
	@echo "  memtest          - Run with valgrind memory checker"
	@echo "  help             - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make test                    # Run all tests"
	@echo "  make debug test              # Run tests with debug build"
	@echo "  make test-collections        # Run only collections tests"
	@echo "  make benchmark               # Run performance tests"