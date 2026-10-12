# FLEET APEX Build Configuration
# Standalone test build - Geode integration separate

CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g
DEBUG_FLAGS = -DDEBUG=1

SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = bin

# Library source files (used by test)
LIB_SOURCES = $(SRCDIR)/Profiler.cpp $(SRCDIR)/LevelAnalyzer.cpp \
              $(SRCDIR)/AdaptiveEngine.cpp $(SRCDIR)/FleetCore.cpp
LIB_OBJECTS = $(LIB_SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

# Test program sources
TEST_SOURCES = $(SRCDIR)/test_main.cpp
TEST_OBJECTS = $(TEST_SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

# Build targets
LIB_TARGET = $(BINDIR)/libfleetapex.a
TEST_TARGET = $(BINDIR)/fleet_apex_test

# Default target - build test program
all: dirs $(TEST_TARGET)

# Create directories
dirs:
	@mkdir -p $(OBJDIR) $(BINDIR)

# Build static library
$(LIB_TARGET): $(LIB_OBJECTS)
	ar rcs $@ $^

# Build test executable
$(TEST_TARGET): $(LIB_OBJECTS) $(TEST_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile library source files
$(OBJDIR)/Profiler.o: $(SRCDIR)/Profiler.cpp
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c -o $@ $<

$(OBJDIR)/LevelAnalyzer.o: $(SRCDIR)/LevelAnalyzer.cpp
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c -o $@ $<

$(OBJDIR)/AdaptiveEngine.o: $(SRCDIR)/AdaptiveEngine.cpp
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c -o $@ $<

$(OBJDIR)/FleetCore.o: $(SRCDIR)/FleetCore.cpp
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c -o $@ $<

$(OBJDIR)/test_main.o: $(SRCDIR)/test_main.cpp
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c -o $@ $<

# Debug build
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: clean all

# Clean
clean:
	rm -rf $(OBJDIR)/*.o $(BINDIR)/*

# Library only
lib: dirs $(LIB_TARGET)

# Test
test: all
	@./$(TEST_TARGET)

# Install (for Geode mod)
install: all
	@echo "Install target - copy to Geode mods directory"

.PHONY: all clean debug install dirs test lib
