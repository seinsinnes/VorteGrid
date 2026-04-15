# Top-level Makefile for MjgIntelFluidDemo_Part20
CXX       := g++

# Build configuration: run "make BUILD=debug" or "make BUILD=release" (default)
BUILD ?= release

# Common flags for both platforms and configurations
CXXFLAGS  := -std=c++23 -Wall -I ./
LDFLAGS   := -lm

# Configuration specific flags
ifeq ($(BUILD),debug)
    CXXFLAGS  += -D_DEBUG -O0 -g -fsanitize=address
    BUILD_DIR := build/debug
else
    CXXFLAGS  += -O3 -ffast-math
    BUILD_DIR := build/release
endif

# OS specific flags
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS specific
    CXXFLAGS += -I/opt/homebrew/include/ -fdeclspec
    LDFLAGS  += -L/opt/homebrew/lib -framework OpenGL -framework GLUT
else
    # Linux (openSUSE) specific
    LDFLAGS  += -lGL -lGLU -lglut
endif

SRC_ROOT  := .
EXEC      := MjgIntelFluidDemo_Part20

# Find all .cpp files in this directory and subdirectories
SRCS      := $(shell find $(SRC_ROOT) -name '*.cpp')
OBJS      := $(patsubst $(SRC_ROOT)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

.PHONY: all clean

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_ROOT)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf build $(EXEC)
