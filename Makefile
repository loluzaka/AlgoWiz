# ALGO VISUALIZER - build with GNU make (MinGW: mingw32-make). C++17, no external libraries.
#   make            build the application      -> build/algoviz(.exe)
#   make test       build and run all tests    -> build/algoviz_tests(.exe)
#   make plugin-dll build the sample plugin    -> build/example_plugin.dll / .so
#   make clean      remove build/
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
LDFLAGS  ?=

ifeq ($(OS),Windows_NT)
  EXE := .exe
  SHARED := .dll
  # Link the C++ runtime statically: a different libstdc++-6.dll earlier on PATH (e.g. Git's
  # mingw64) would otherwise be loaded at run time and crash std::fstream.
  LDFLAGS += -static
  SHAREDFLAGS := -static-libstdc++ -static-libgcc
else
  EXE :=
  SHARED := .so
  LDFLAGS += -ldl
  SHAREDFLAGS :=
endif

# Every .cpp under src/ except the sample plugin (that is built separately).
LIB_SRC  := $(filter-out src/plugin/examples/ExampleDll.cpp,$(wildcard src/*.cpp src/*/*.cpp src/*/*/*.cpp))
TEST_SRC := $(wildcard tests/*.cpp tests/*/*.cpp)
LIB_OBJ  := $(patsubst %.cpp,build/%.o,$(LIB_SRC))
TEST_OBJ := $(patsubst %.cpp,build/%.o,$(TEST_SRC))
MAIN_OBJ := build/main.o
APP      := build/algoviz$(EXE)
TESTBIN  := build/algoviz_tests$(EXE)

.PHONY: all test plugin-dll clean
all: $(APP)

$(APP): $(MAIN_OBJ) $(LIB_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(TESTBIN): $(TEST_OBJ) $(LIB_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

test: $(TESTBIN) plugin-dll
	./$(TESTBIN)

plugin-dll: build/example_plugin$(SHARED)
build/example_plugin$(SHARED): src/plugin/examples/ExampleDll.cpp $(wildcard src/plugin/*.h src/engine/*.h)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -shared -fPIC $(SHAREDFLAGS) -o $@ src/plugin/examples/ExampleDll.cpp

build/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -rf build

-include $(LIB_OBJ:.o=.d) $(TEST_OBJ:.o=.d) $(MAIN_OBJ:.o=.d)
