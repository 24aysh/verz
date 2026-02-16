# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -I./include
LDFLAGS := -lz -lssl -lcrypto

# Directories
SRC_DIR := src
CMD_DIR := $(SRC_DIR)/cmd
INCLUDE_DIR := include
BIN_DIR := bin
OBJ_DIR := $(BIN_DIR)/obj

# Target executable
TARGET := $(BIN_DIR)/verz

# Source files
MAIN_SRC := $(SRC_DIR)/main.cpp
CMD_SRCS := $(wildcard $(CMD_DIR)/*.cpp)
SRCS := $(MAIN_SRC) $(CMD_SRCS)

# Object files
MAIN_OBJ := $(OBJ_DIR)/main.o
CMD_OBJS := $(patsubst $(CMD_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(CMD_SRCS))
OBJS := $(MAIN_OBJ) $(CMD_OBJS)

# Header files (for dependency tracking)
HEADERS := $(wildcard $(INCLUDE_DIR)/*.h) $(wildcard $(INCLUDE_DIR)/cmd/*.h)

# Default target
.PHONY: all
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJS) | $(BIN_DIR)
	@echo "Linking $@..."
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Build complete: $@"

# Compile main.cpp
$(OBJ_DIR)/main.o: $(MAIN_SRC) $(HEADERS) | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile command source files
$(OBJ_DIR)/%.o: $(CMD_DIR)/%.cpp $(HEADERS) | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create directories if they don't exist
$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BIN_DIR)
	@echo "Clean complete"

# Run the program
.PHONY: run
run: $(TARGET)
	@$(TARGET)

# Install to system (optional, requires sudo)
.PHONY: install
install: $(TARGET)
	@echo "Installing verz to /usr/local/bin..."
	@sudo cp $(TARGET) /usr/local/bin/verz
	@echo "Installation complete"

# Uninstall from system
.PHONY: uninstall
uninstall:
	@echo "Uninstalling verz from /usr/local/bin..."
	@sudo rm -f /usr/local/bin/verz
	@echo "Uninstall complete"

# Rebuild everything
.PHONY: rebuild
rebuild: clean all

# Debug build with debug symbols
.PHONY: debug
debug: CXXFLAGS += -g -O0 -DDEBUG
debug: clean all

# Release build with optimizations
.PHONY: release
release: CXXFLAGS += -O3 -DNDEBUG
release: clean all

# Show help
.PHONY: help
help:
	@echo "Verz Build System"
	@echo "================="
	@echo "Available targets:"
	@echo "  all       - Build the project (default)"
	@echo "  clean     - Remove build artifacts"
	@echo "  rebuild   - Clean and build"
	@echo "  debug     - Build with debug symbols"
	@echo "  release   - Build with optimizations"
	@echo "  run       - Build and run the program"
	@echo "  install   - Install to /usr/local/bin (requires sudo)"
	@echo "  uninstall - Remove from /usr/local/bin (requires sudo)"
	@echo "  help      - Show this help message"

# Print variables for debugging the Makefile
.PHONY: print-vars
print-vars:
	@echo "CXX       = $(CXX)"
	@echo "CXXFLAGS  = $(CXXFLAGS)"
	@echo "LDFLAGS   = $(LDFLAGS)"
	@echo "SRCS      = $(SRCS)"
	@echo "OBJS      = $(OBJS)"
	@echo "TARGET    = $(TARGET)"
