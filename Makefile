# Fent Compiler Makefile
# Builds compiler, tests, and examples

CXX := c++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
DEBUG_FLAGS := -g -DDEBUG
TEST_FLAGS := -std=c++17 -Wall -Wextra -g

ARCH := $(shell uname -m)
OS := $(shell uname -s | tr A-Z a-z)

SRC_DIR := src
TEST_DIR := tests
BUILD_DIR := build
BIN_DIR := bin/$(ARCH)
OBJ_DIR := $(BUILD_DIR)/obj/$(ARCH)
TEST_BIN_DIR := $(BUILD_DIR)/tests/$(ARCH)

LEXER_SRC := $(SRC_DIR)/lexer.cpp
AST_SRC := $(SRC_DIR)/ast.cpp
CODEGEN_SRC := $(SRC_DIR)/CogeGen/x86_64.cpp

CORE_SRCS := $(LEXER_SRC) $(AST_SRC) $(CODEGEN_SRC)
CORE_OBJS := $(OBJ_DIR)/lexer.o $(OBJ_DIR)/ast.o $(OBJ_DIR)/x86_64.o

MAIN_BIN := $(BIN_DIR)/fentc

TEST_FILES := $(wildcard $(TEST_DIR)/*.fent)

NASM := nasm
NASM_FLAGS := -f elf64
LD := ld

.PHONY: all
all: $(MAIN_BIN)

$(BIN_DIR) $(OBJ_DIR) $(TEST_BIN_DIR):
	@mkdir -p $@

$(OBJ_DIR)/lexer.o: $(LEXER_SRC) $(SRC_DIR)/lexer.hpp $(SRC_DIR)/token.hpp | $(OBJ_DIR)
	@echo "[CC] Compiling lexer..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/ast.o: $(AST_SRC) $(SRC_DIR)/ast.hpp $(SRC_DIR)/lexer.hpp | $(OBJ_DIR)
	@echo "[CC] Compiling AST..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/x86_64.o: $(CODEGEN_SRC) $(SRC_DIR)/code_gen.hpp $(SRC_DIR)/ast.hpp | $(OBJ_DIR)
	@echo "[CC] Compiling code generator..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(MAIN_BIN): main.cpp $(CORE_OBJS) | $(BIN_DIR)
	@echo "[LD] Linking fent compiler ($(ARCH))..."
	@$(CXX) $(CXXFLAGS) main.cpp $(CORE_OBJS) -o $@
	@echo "Compiler built: $(MAIN_BIN)"

.PHONY: test
test: $(MAIN_BIN)
	@echo "Running test suite..."
	@PASS=0; FAIL=0; TOTAL=0; \
	for test in $(TEST_FILES); do \
		TOTAL=$$((TOTAL + 1)); \
		TEST_NAME=$$(basename $$test); \
		printf "  [%2d/15] %-25s " $$TOTAL "$$TEST_NAME"; \
		if ./$(MAIN_BIN) $$test -o /dev/null 2>&1 | grep -q "Parsed"; then \
			echo "✓ PASS"; \
			PASS=$$((PASS + 1)); \
		else \
			echo "✗ FAIL"; \
			FAIL=$$((FAIL + 1)); \
		fi; \
	done; \
	echo ""; \
	echo "Results: $$PASS/$$TOTAL passed, $$FAIL failed"

.PHONY: test-verbose
test-verbose: $(MAIN_BIN)
	@echo "Running test suite (verbose)..."
	@for test in $(TEST_FILES); do \
		echo ""; \
		echo "Testing $$(basename $$test):"; \
		./$(MAIN_BIN) $$test -o /dev/null; \
	done

.PHONY: test-single
test-single: $(MAIN_BIN)
	@if [ -z "$(FILE)" ]; then \
		echo "Error: Please specify FILE=<test_name.fent>"; \
		echo "Example: make test-single FILE=01_literals.fent"; \
		exit 1; \
	fi; \
	echo "Testing $(FILE)..."; \
	./$(MAIN_BIN) $(TEST_DIR)/$(FILE) -l -a

.PHONY: test-compile
test-compile: $(MAIN_BIN)
	@if [ -z "$(FILE)" ]; then \
		echo "Error: Please specify FILE=<test_name.fent>"; \
		echo "Example: make test-compile FILE=09_recursion.fent"; \
		exit 1; \
	fi; \
	BASENAME=$$(basename $(FILE) .fent); \
	echo "Compiling and running $(FILE)..."; \
	./$(MAIN_BIN) $(TEST_DIR)/$(FILE) -o $$BASENAME.asm && \
	$(NASM) $(NASM_FLAGS) $$BASENAME.asm -o $$BASENAME.o && \
	$(LD) $$BASENAME.o -o $$BASENAME && \
	echo "Running $$BASENAME..."; \
	./$$BASENAME; \
	EXIT_CODE=$$?; \
	rm -f $$BASENAME.asm $$BASENAME.o $$BASENAME; \
	echo "Exit code: $$EXIT_CODE"

.PHONY: debug
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: clean all
	@echo "Debug build complete"

.PHONY: clean
clean:
	@echo "[CLEAN] Removing build artifacts..."
	@rm -rf $(BUILD_DIR)
	@rm -rf bin
	@rm -f output.asm output.o output
	@rm -f tokens_*.txt ast_*.txt
	@echo "Clean complete"

.PHONY: clean-debug
clean-debug:
	@echo "[CLEAN] Removing debug output files..."
	@rm -f tokens_*.txt ast_*.txt
	@echo "Debug files cleaned"

# Compile a .fent file to executable
.PHONY: compile
compile:
	@if [ -z "$(FILE)" ]; then \
		echo "Error: Please specify FILE=<file.fent>"; \
		exit 1; \
	fi; \
	BASENAME=$$(basename $(FILE) .fent); \
	echo "Compiling $(FILE)..."; \
	./$(MAIN_BIN) $(FILE) -o $$BASENAME.asm && \
	$(NASM) $(NASM_FLAGS) $$BASENAME.asm -o $$BASENAME.o && \
	$(LD) $$BASENAME.o -o $$BASENAME && \
	rm -f $$BASENAME.asm $$BASENAME.o && \
	echo "Executable created: $$BASENAME"

.PHONY: install
install: $(MAIN_BIN)
	@echo "Installing fentc..."
	@if [ -z "$(PREFIX)" ]; then \
		echo "Using default PREFIX=/usr/local"; \
		PREFIX=/usr/local; \
	fi; \
	install -d $$PREFIX/bin && \
	install -m 755 $(MAIN_BIN) $$PREFIX/bin/fentc && \
	echo "Installed to $$PREFIX/bin/fentc"

.PHONY: info
info:
	@echo "Fent Compiler Build Configuration"
	@echo "  Architecture: $(ARCH)"
	@echo "  OS:           $(OS)"
	@echo "  Compiler:     $(CXX)"
	@echo "  Flags:        $(CXXFLAGS)"
	@echo "  Output:       $(BIN_DIR)"
	@echo "  Test Files:   $(words $(TEST_FILES)) .fent files"
	@echo ""
	@echo "Available Targets:"
	@echo "  make                - Build compiler"
	@echo "  make test           - Run all test files"
	@echo "  make test-verbose   - Run all tests with verbose output"
	@echo "  make test-single FILE=<name.fent> - Test single file with debug output"
	@echo "  make test-compile FILE=<name.fent> - Compile and run a test file"
	@echo "  make compile FILE=<file.fent>      - Compile a .fent file to executable"
	@echo "  make install [PREFIX=/path]        - Install compiler"
	@echo "  make debug          - Build with debug flags"
	@echo "  make clean          - Remove all build artifacts"
	@echo "  make clean-debug    - Remove debug output files (tokens_*, ast_*)"
	@echo "  make info           - Show this information"
	@echo ""
	@echo "Examples:"
	@echo "  make test"
	@echo "  make test-single FILE=09_recursion.fent"
	@echo "  make test-compile FILE=15_complex_program.fent"
	@echo "  make compile FILE=myprogram.fent"

.PHONY: help
help: info

.PHONY: all clean clean-debug debug info help test test-verbose test-single test-compile compile install
