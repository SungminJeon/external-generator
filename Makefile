# Makefile for external_generator
# Enhanced topology generator with External curve support

# ============================================================================
# Compiler Configuration
# ============================================================================
CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -Wpedantic
LDFLAGS = 

# Eigen include path (adjust for your system)
# macOS with Homebrew
EIGEN_INCLUDE = -I/opt/homebrew/include/eigen3
# Linux alternatives:
# EIGEN_INCLUDE = -I/usr/include/eigen3
# EIGEN_INCLUDE = -I/usr/local/include/eigen3

INCLUDES = $(EIGEN_INCLUDE)

# ============================================================================
# Target Configuration
# ============================================================================
TARGET = external_generator

# ============================================================================
# Source Files
# ============================================================================

# Main program
MAIN_SRC = external_generator.cpp

# Enhanced topology system (required)
ENHANCED_SRC = \
	Topology_enhanced.cpp \
	TopologyDB_enhanced.cpp \
	TopoLineCompact_enhanced.cpp

# Basic topology system (optional, for backward compatibility)
BASIC_SRC = \
	Topology.cpp \
	TopologyDB.cpp \
	TopoLineCompact.cpp

# Tensor/Theory system
TENSOR_SRC = Tensor.C

# Combined source list
SRC = $(MAIN_SRC) $(ENHANCED_SRC) $(TENSOR_SRC)

# Object files
OBJ = $(SRC:.cpp=.o)
OBJ := $(OBJ:.C=.o)

# Dependency files
DEP = $(OBJ:.o=.d)

# ============================================================================
# Build Rules
# ============================================================================

.PHONY: all clean distclean help test

# Default target
all: $(TARGET)

# Link target
$(TARGET): $(OBJ)
	@echo "Linking $@..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build complete: $@"

# Compile C++ source files
%.o: %.cpp
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

# Compile C source files (Tensor.C)
%.o: %.C
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

# ============================================================================
# Utility Targets
# ============================================================================

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(OBJ) $(DEP)
	@echo "Clean complete"

# Clean everything including target
distclean: clean
	@echo "Removing target..."
	rm -f $(TARGET)
	@echo "Distclean complete"

# Rebuild from scratch
rebuild: distclean all

# Show help
help:
	@echo "Available targets:"
	@echo "  all        - Build the external_generator (default)"
	@echo "  clean      - Remove object and dependency files"
	@echo "  distclean  - Remove all build artifacts including executable"
	@echo "  rebuild    - Clean and rebuild from scratch"
	@echo "  test       - Run quick compilation test"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Usage examples:"
	@echo "  make              # Build with default settings"
	@echo "  make clean all    # Clean and rebuild"
	@echo "  make CXX=clang++  # Build with clang instead of g++"
	@echo ""
	@echo "Configuration:"
	@echo "  CXX       = $(CXX)"
	@echo "  CXXFLAGS  = $(CXXFLAGS)"
	@echo "  INCLUDES  = $(INCLUDES)"

# Quick compilation test
test: $(TARGET)
	@echo "Running quick test..."
	@./$(TARGET) -h || true
	@echo "Test complete"

# Show detected files
info:
	@echo "=== Build Configuration ==="
	@echo "Target:       $(TARGET)"
	@echo "Compiler:     $(CXX)"
	@echo "Flags:        $(CXXFLAGS)"
	@echo "Includes:     $(INCLUDES)"
	@echo ""
	@echo "=== Source Files ==="
	@echo "$(SRC)" | tr ' ' '\n'
	@echo ""
	@echo "=== Object Files ==="
	@echo "$(OBJ)" | tr ' ' '\n'

# ============================================================================
# Dependency Tracking
# ============================================================================

# Include dependency files if they exist
-include $(DEP)

# ============================================================================
# Platform-specific Configuration
# ============================================================================

# Detect platform and adjust settings
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
    # macOS
    EIGEN_INCLUDE ?= -I/opt/homebrew/include/eigen3
    CXXFLAGS += -stdlib=libc++
endif

ifeq ($(UNAME_S),Linux)
    # Linux
    EIGEN_INCLUDE ?= -I/usr/include/eigen3
endif

# ============================================================================
# Optional: Debug Build
# ============================================================================

# Build with debug symbols
debug: CXXFLAGS = -std=c++17 -O0 -g -Wall -Wextra -Wpedantic -DDEBUG
debug: clean all
	@echo "Debug build complete"

# ============================================================================
# Optional: Profile Build
# ============================================================================

# Build with profiling enabled
profile: CXXFLAGS = -std=c++17 -O3 -g -pg -Wall -Wextra
profile: clean all
	@echo "Profile build complete"

# ============================================================================
# Installation (optional)
# ============================================================================

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

install: $(TARGET)
	@echo "Installing $(TARGET) to $(BINDIR)..."
	install -d $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)
	@echo "Installation complete"

uninstall:
	@echo "Uninstalling $(TARGET) from $(BINDIR)..."
	rm -f $(BINDIR)/$(TARGET)
	@echo "Uninstall complete"
