CC = gcc
VERSION = $(shell cat VERSION)
CFLAGS = -Wall -Wextra -std=c99 -Iinclude -DEMBER_VERSION_STRING=\"$(VERSION)\"
DEBUG_FLAGS = -g -O0 -DDEBUG
RELEASE_FLAGS = -O2 -DNDEBUG
ASAN_FLAGS = -g -O1 -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -DDEBUG
COVERAGE_FLAGS = -g -O0 -DDEBUG -fprofile-arcs -ftest-coverage

# Thread optimization flags
THREAD_OPT_FLAGS = -pthread -std=c11 -D_GNU_SOURCE -mavx2 -mfma
LOCKFREE_FLAGS = -DENABLE_LOCKFREE_OPTIMIZATIONS -DATOMIC_OPERATIONS_SUPPORT -DUSE_LOCKFREE_VM_POOL
PERFORMANCE_FLAGS = -O3 -DNDEBUG -march=native -mtune=native -flto -fomit-frame-pointer
VM_POOL_FLAGS = -DENABLE_VM_POOL_LOCKFREE -DENABLE_ADAPTIVE_POOL_SIZING -lnuma

# Check for readline library availability
HAVE_READLINE := $(shell pkg-config --exists readline 2>/dev/null && echo 1 || echo 0)
ifeq ($(HAVE_READLINE),1)
    CFLAGS += -DHAVE_READLINE
    READLINE_LIBS = $(shell pkg-config --libs readline)
else
    # Fallback: try to find readline without pkg-config
    HAVE_READLINE := $(shell echo '#include <readline/readline.h>' | $(CC) -E - >/dev/null 2>&1 && echo 1 || echo 0)
    ifeq ($(HAVE_READLINE),1)
        CFLAGS += -DHAVE_READLINE
        READLINE_LIBS = -lreadline
    else
        READLINE_LIBS =
    endif
endif

# Check for libcurl availability
HAVE_CURL := $(shell pkg-config --exists libcurl 2>/dev/null && echo 1 || echo 0)
ifeq ($(HAVE_CURL),1)
    CFLAGS += -DHAVE_CURL
    CURL_LIBS = $(shell pkg-config --libs libcurl)
else
    # Fallback: try to find curl without pkg-config
    HAVE_CURL := $(shell echo '#include <curl/curl.h>' | $(CC) -E - >/dev/null 2>&1 && echo 1 || echo 0)
    ifeq ($(HAVE_CURL),1)
        CFLAGS += -DHAVE_CURL
        CURL_LIBS = -lcurl
    else
        CURL_LIBS =
    endif
endif

# Default to release build
BUILD_TYPE ?= release
ifeq ($(BUILD_TYPE),debug)
    CFLAGS += $(DEBUG_FLAGS)
else ifeq ($(BUILD_TYPE),asan)
    CFLAGS += $(ASAN_FLAGS)
    LDFLAGS = -fsanitize=address -fsanitize=undefined
else ifeq ($(BUILD_TYPE),coverage)
    CFLAGS += $(COVERAGE_FLAGS)
    LDFLAGS = -lgcov --coverage
else
    CFLAGS += $(RELEASE_FLAGS)
endif

SRCDIR = src
TOOLSDIR = tools
TESTSDIR = tests/unit
FUZZDIR = tests/fuzz
BUILDDIR = build

# Core library
LIBNAME = libember.a

# Module directories
CORE_DIR = $(SRCDIR)/core
FRONTEND_DIR = $(SRCDIR)/frontend
RUNTIME_DIR = $(SRCDIR)/runtime

# Core library source files
FRONTEND_MODULES = $(FRONTEND_DIR)/lexer/lexer.c $(FRONTEND_DIR)/parser/parser.c $(FRONTEND_DIR)/parser/core.c $(FRONTEND_DIR)/parser/expressions.c $(FRONTEND_DIR)/parser/statements.c $(FRONTEND_DIR)/parser/oop.c
CORE_MODULES = $(CORE_DIR)/vm.c $(CORE_DIR)/vm_arithmetic.c $(CORE_DIR)/vm_comparison.c $(CORE_DIR)/vm_stack.c $(CORE_DIR)/string_intern_optimized.c $(CORE_DIR)/bytecode.c $(CORE_DIR)/memory.c $(CORE_DIR)/error.c $(CORE_DIR)/optimizer.c $(CORE_DIR)/memory/memory_pool.c $(CORE_DIR)/vm_pool/concurrent_vm_pool.c $(CORE_DIR)/vm_pool/vm_pool_lockfree.c $(CORE_DIR)/vm_pool/work_stealing_pool.c $(CORE_DIR)/vm_pool/vm_memory_integration.c
RUNTIME_MODULES = $(RUNTIME_DIR)/builtins.c $(RUNTIME_DIR)/value/value.c $(RUNTIME_DIR)/vfs/vfs.c $(RUNTIME_DIR)/package/package.c

LIBSRC = $(SRCDIR)/api.c $(FRONTEND_MODULES) $(CORE_MODULES) $(RUNTIME_MODULES)

# Core library object files
LIBOBJ = $(BUILDDIR)/api.o
LIBOBJ += $(BUILDDIR)/lexer.o $(BUILDDIR)/parser.o $(BUILDDIR)/parser_core.o $(BUILDDIR)/parser_expressions.o $(BUILDDIR)/parser_statements.o $(BUILDDIR)/parser_oop.o
LIBOBJ += $(BUILDDIR)/core_vm.o $(BUILDDIR)/core_vm_arithmetic.o $(BUILDDIR)/core_vm_comparison.o $(BUILDDIR)/core_vm_stack.o $(BUILDDIR)/core_string_intern_optimized.o $(BUILDDIR)/core_bytecode.o $(BUILDDIR)/core_memory.o $(BUILDDIR)/core_error.o $(BUILDDIR)/core_optimizer.o $(BUILDDIR)/core_memory_memory_pool.o $(BUILDDIR)/core_vm_pool_concurrent_vm_pool.o $(BUILDDIR)/core_vm_pool_vm_pool_lockfree.o $(BUILDDIR)/core_vm_pool_work_stealing_pool.o $(BUILDDIR)/core_vm_pool_vm_memory_integration.o
LIBOBJ += $(BUILDDIR)/runtime_builtins.o $(BUILDDIR)/value.o $(BUILDDIR)/vfs.o $(BUILDDIR)/package.o

# Core tools (essential tools only)
CORE_TOOLS = ember emberc
CORE_TOOL_BINS = $(addprefix $(BUILDDIR)/, $(CORE_TOOLS))

# Core tests (essential tests only)
CORE_TESTS = test-vm test-lexer-basic test-parser-core test-parser-expressions test-parser-statements test-builtins test-value test-package test-basic-ops test-simple test-minimal
CORE_TEST_BINS = $(addprefix $(BUILDDIR)/, $(CORE_TESTS))

# Fuzzing tests
FUZZ_TESTS = fuzz-parser fuzz-vm fuzz-comprehensive
FUZZ_BINS = $(addprefix $(BUILDDIR)/, $(FUZZ_TESTS))

.PHONY: all clean tools tests fuzz fuzz-run debug release asan coverage install check help

all: $(BUILDDIR)/$(LIBNAME) tools tests

# Create build directory
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# Core library
$(BUILDDIR)/$(LIBNAME): $(LIBOBJ) | $(BUILDDIR)
	ar rcs $@ $^

# Build object files
$(BUILDDIR)/api.o: $(SRCDIR)/api.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Frontend modules
$(BUILDDIR)/lexer.o: $(FRONTEND_DIR)/lexer/lexer.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/parser.o: $(FRONTEND_DIR)/parser/parser.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/parser_core.o: $(FRONTEND_DIR)/parser/core.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/parser_expressions.o: $(FRONTEND_DIR)/parser/expressions.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/parser_statements.o: $(FRONTEND_DIR)/parser/statements.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/parser_oop.o: $(FRONTEND_DIR)/parser/oop.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Core modules
$(BUILDDIR)/core_vm.o: $(CORE_DIR)/vm.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/core_vm_arithmetic.o: $(CORE_DIR)/vm_arithmetic.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/core_vm_comparison.o: $(CORE_DIR)/vm_comparison.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/core_vm_stack.o: $(CORE_DIR)/vm_stack.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/core_string_intern_optimized.o: $(CORE_DIR)/string_intern_optimized.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/core_bytecode.o: $(CORE_DIR)/bytecode.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/core_memory.o: $(CORE_DIR)/memory.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/core_error.o: $(CORE_DIR)/error.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/core_optimizer.o: $(CORE_DIR)/optimizer.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/core_memory_memory_pool.o: $(CORE_DIR)/memory/memory_pool.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(THREAD_OPT_FLAGS) -c $< -o $@

$(BUILDDIR)/core_vm_pool_concurrent_vm_pool.o: $(CORE_DIR)/vm_pool/concurrent_vm_pool.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(THREAD_OPT_FLAGS) $(LOCKFREE_FLAGS) $(VM_POOL_FLAGS) -c $< -o $@

$(BUILDDIR)/core_vm_pool_vm_pool_lockfree.o: $(CORE_DIR)/vm_pool/vm_pool_lockfree.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(THREAD_OPT_FLAGS) $(LOCKFREE_FLAGS) $(VM_POOL_FLAGS) -c $< -o $@

$(BUILDDIR)/core_vm_pool_work_stealing_pool.o: $(CORE_DIR)/vm_pool/work_stealing_pool.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(THREAD_OPT_FLAGS) $(LOCKFREE_FLAGS) $(VM_POOL_FLAGS) -c $< -o $@

$(BUILDDIR)/core_vm_pool_vm_memory_integration.o: $(CORE_DIR)/vm_pool/vm_memory_integration.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(THREAD_OPT_FLAGS) -c $< -o $@

# Runtime modules
$(BUILDDIR)/runtime_builtins.o: $(RUNTIME_DIR)/builtins.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/value.o: $(RUNTIME_DIR)/value/value.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/vfs.o: $(RUNTIME_DIR)/vfs/vfs.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/package.o: $(RUNTIME_DIR)/package/package.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Core tools
tools: $(CORE_TOOL_BINS)

$(BUILDDIR)/ember: $(TOOLSDIR)/ember/ember.c $(BUILDDIR)/$(LIBNAME) | $(BUILDDIR)
	$(CC) $(CFLAGS) $< -L$(BUILDDIR) -lember -lm -lpthread -lssl -lcrypto $(READLINE_LIBS) $(CURL_LIBS) $(LDFLAGS) -o $@

$(BUILDDIR)/emberc: $(TOOLSDIR)/emberc/emberc.c $(BUILDDIR)/$(LIBNAME) | $(BUILDDIR)
	$(CC) $(CFLAGS) $< -L$(BUILDDIR) -lember -lm -lssl -lcrypto $(CURL_LIBS) $(LDFLAGS) -o $@

# Core tests
tests: $(CORE_TEST_BINS)

$(BUILDDIR)/test-vm: $(TESTSDIR)/test_vm.c $(BUILDDIR)/$(LIBNAME) | $(BUILDDIR)
	$(CC) $(CFLAGS) $< -L$(BUILDDIR) -lember -lm -lssl -lcrypto $(CURL_LIBS) $(LDFLAGS) -o $@

$(BUILDDIR)/test-lexer-basic: $(TESTSDIR)/test_lexer_basic.c $(BUILDDIR)/$(LIBNAME) | $(BUILDDIR)
	$(CC) $(CFLAGS) $< -L$(BUILDDIR) -lember -lm -lssl -lcrypto $(CURL_LIBS) $(LDFLAGS) -o $@

$(BUILDDIR)/test-parser-core: $(TESTSDIR)/test_parser_core.c $(BUILDDIR)/$(LIBNAME) | $(BUILDDIR)
	$(CC) $(CFLAGS) $< -L$(BUILDDIR) -lember -lm -lssl -lcrypto $(CURL_LIBS) $(LDFLAGS) -o $@

$(BUILDDIR)/test-parser-expressions: $(TESTSDIR)/test_parser_expressions.c $(BUILDDIR)/$(LIBNAME) | $(BUILDDIR)
	$(CC) $(CFLAGS) $< -L$(BUILDDIR) -lember -lm -lssl -lcrypto $(CURL_LIBS) $(LDFLAGS) -o $@

$(BUILDDIR)/test-parser-statements: $(TESTSDIR)/test_parser_statements.c $(BUILDDIR)/$(LIBNAME) | $(BUILDDIR)
	$(CC) $(CFLAGS) $< -L$(BUILDDIR) -lember -lm -lssl -lcrypto $(CURL_LIBS) $(LDFLAGS) -o $@

$(BUILDDIR)/test-builtins: $(TESTSDIR)/test_builtins.c $(BUILDDIR)/$(LIBNAME) | $(BUILDDIR)
	$(CC) $(CFLAGS) $< -L$(BUILDDIR) -lember -lm -lssl -lcrypto $(CURL_LIBS) $(LDFLAGS) -o $@

$(BUILDDIR)/test-value: $(TESTSDIR)/test_value.c $(BUILDDIR)/$(LIBNAME) | $(BUILDDIR)
	$(CC) $(CFLAGS) $< -L$(BUILDDIR) -lember -lm -lssl -lcrypto $(CURL_LIBS) $(LDFLAGS) -o $@

$(BUILDDIR)/test-package: $(TESTSDIR)/test_package.c $(BUILDDIR)/$(LIBNAME) | $(BUILDDIR)
	$(CC) $(CFLAGS) $< -L$(BUILDDIR) -lember -lm -lssl -lcrypto $(CURL_LIBS) $(LDFLAGS) -o $@

$(BUILDDIR)/test-basic-ops: $(TESTSDIR)/test_basic_ops.c $(BUILDDIR)/$(LIBNAME) | $(BUILDDIR)
	$(CC) $(CFLAGS) $< -L$(BUILDDIR) -lember -lm -lssl -lcrypto $(CURL_LIBS) $(LDFLAGS) -o $@

$(BUILDDIR)/test-simple: $(TESTSDIR)/test_simple.c $(BUILDDIR)/$(LIBNAME) | $(BUILDDIR)
	$(CC) $(CFLAGS) $< -L$(BUILDDIR) -lember -lm -lssl -lcrypto $(CURL_LIBS) $(LDFLAGS) -o $@

$(BUILDDIR)/test-minimal: $(TESTSDIR)/test_minimal.c $(BUILDDIR)/$(LIBNAME) | $(BUILDDIR)
	$(CC) $(CFLAGS) $< -L$(BUILDDIR) -lember -lm -lssl -lcrypto $(CURL_LIBS) $(LDFLAGS) -o $@

# Fuzzing tests
fuzz: $(FUZZ_BINS)

$(BUILDDIR)/fuzz-parser: $(FUZZDIR)/fuzz_parser.c $(BUILDDIR)/$(LIBNAME) | $(BUILDDIR)
	$(CC) $(CFLAGS) $< -L$(BUILDDIR) -lember -lm -lssl -lcrypto $(CURL_LIBS) $(LDFLAGS) -o $@

$(BUILDDIR)/fuzz-vm: $(FUZZDIR)/fuzz_vm.c $(BUILDDIR)/$(LIBNAME) | $(BUILDDIR)
	$(CC) $(CFLAGS) $< -L$(BUILDDIR) -lember -lm -lssl -lcrypto $(CURL_LIBS) $(LDFLAGS) -o $@

$(BUILDDIR)/fuzz-comprehensive: $(FUZZDIR)/fuzz_comprehensive.c $(FUZZDIR)/fuzz_common.c $(BUILDDIR)/$(LIBNAME) | $(BUILDDIR)
	$(CC) $(CFLAGS) $(FUZZDIR)/fuzz_comprehensive.c $(FUZZDIR)/fuzz_common.c -I$(FUZZDIR) -L$(BUILDDIR) -lember -lm -lssl -lcrypto $(CURL_LIBS) $(LDFLAGS) -o $@

# Build variants
debug:
	$(MAKE) BUILD_TYPE=debug

release:
	$(MAKE) BUILD_TYPE=release

asan:
	$(MAKE) BUILD_TYPE=asan

coverage:
	$(MAKE) BUILD_TYPE=coverage

# Run tests
check: tests
	@echo "Running core Ember language tests..."
	$(BUILDDIR)/test-vm
	$(BUILDDIR)/test-lexer-basic
	$(BUILDDIR)/test-parser-core
	$(BUILDDIR)/test-parser-expressions
	$(BUILDDIR)/test-parser-statements
	$(BUILDDIR)/test-builtins
	$(BUILDDIR)/test-value
	$(BUILDDIR)/test-package
	$(BUILDDIR)/test-basic-ops
	$(BUILDDIR)/test-simple
	$(BUILDDIR)/test-minimal

# Run fuzzing tests
fuzz-run: fuzz
	@echo "Running fuzzing tests..."
	$(BUILDDIR)/fuzz-comprehensive -n 1000
	$(BUILDDIR)/fuzz-parser 500
	$(BUILDDIR)/fuzz-vm 500

# Installation
PREFIX ?= /usr/local
install: all
	install -d $(PREFIX)/lib $(PREFIX)/include $(PREFIX)/bin
	install $(BUILDDIR)/$(LIBNAME) $(PREFIX)/lib/
	install include/ember.h $(PREFIX)/include/
	install $(CORE_TOOL_BINS) $(PREFIX)/bin/

# Cleanup
clean:
	rm -rf $(BUILDDIR)

# Show readline library status
readline-info:
	@echo "Readline library detection:"
ifeq ($(HAVE_READLINE),1)
	@echo "  Status: ✓ Available"
	@echo "  Flags: -DHAVE_READLINE"
	@echo "  Libraries: $(READLINE_LIBS)"
	@echo "  Features: Tab completion, command history, line editing"
else
	@echo "  Status: ✗ Not available"
	@echo "  Note: Install readline development libraries for enhanced REPL features"
	@echo "  Ubuntu/Debian: sudo apt-get install libreadline-dev"
	@echo "  Fedora/RHEL: sudo dnf install readline-devel"
	@echo "  Arch Linux: sudo pacman -S readline"
	@echo "  macOS: brew install readline"
endif

# Help target
help:
	@echo "Ember Core Language Build System"
	@echo "==============================="
	@echo ""
	@echo "Main Targets:"
	@echo "  all              - Build everything (library, tools, tests)"
	@echo "  clean            - Remove all build files"
	@echo "  install          - Install Ember Core to $(PREFIX)"
	@echo ""
	@echo "Core Components:"
	@echo "  tools            - Build Ember REPL and compiler"
	@echo "  tests            - Build core test programs"
	@echo "  check            - Run test suite"
	@echo "  fuzz             - Build fuzzing tests"
	@echo "  fuzz-run         - Run fuzzing tests"
	@echo ""
	@echo "Build Variants:"
	@echo "  debug            - Build with debug symbols"
	@echo "  release          - Build optimized version (default)"
	@echo "  asan             - Build with AddressSanitizer"
	@echo "  coverage         - Build with coverage instrumentation"
	@echo ""
	@echo "Tools:"
	@echo "  ember            - Interactive REPL"
	@echo "  emberc           - Ember compiler"
	@echo ""
	@echo "Utilities:"
	@echo "  readline-info    - Show readline library status"
	@echo "  help             - Show this help message"