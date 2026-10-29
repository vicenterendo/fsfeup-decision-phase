# ============================================================================
# Makefile.cpp  -  build and run the MCU program (main.cpp).
#
#   make -f Makefile.cpp              # same as "build"
#   make -f Makefile.cpp build        # compile main.cpp -> ./mcu
#   make -f Makefile.cpp run          # compile (if needed) and run ./mcu
#   make -f Makefile.cpp clean        # delete ./mcu
#
# Needs: g++ (C++17) and the nlohmann-json header library. See README.md.
# Start vm_server.py first (see Makefile.python), then run the MCU.
# ============================================================================

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
TARGET   := mcu
SRC 		 := src/env/main.cpp src/env/vm_io.hpp src/*.cpp
HEADERS  := src/*.h src/*.hpp src/signal/*

# On macOS, Homebrew installs headers outside the default search path
ifeq ($(shell uname -s),Darwin)
  BREW_PREFIX := $(shell brew --prefix 2>/dev/null)
  ifneq ($(BREW_PREFIX),)
    CXXFLAGS += -I$(BREW_PREFIX)/include
  endif
endif

.PHONY: build run clean

build: $(TARGET)

$(TARGET): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
