# Algo Visualizer - CMake build (C++17, no external libraries).
# NOTE: the project is developed and tested with the Makefile (see Documentation/Project_Wide/Build_Guide.md).
# This file is provided for CMake users; it was written to mirror the Makefile exactly.
cmake_minimum_required(VERSION 3.16)
project(AlgoVisualizer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# ---- library with everything except main.cpp (shared by the app, the tests and nothing else)
file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)
list(FILTER LIB_SOURCES EXCLUDE REGEX "src/plugin/examples/ExampleDll\.cpp$")
add_library(algoviz_core STATIC ${LIB_SOURCES})
target_include_directories(algoviz_core PUBLIC ${CMAKE_SOURCE_DIR})
if(MSVC)
  target_compile_options(algoviz_core PUBLIC /W3 /utf-8)
else()
  target_compile_options(algoviz_core PUBLIC -Wall -Wextra)
endif()
if(UNIX)
  target_link_libraries(algoviz_core PUBLIC ${CMAKE_DL_LIBS})
endif()
if(MINGW)
  target_link_options(algoviz_core PUBLIC -static)   # avoid picking up a foreign libstdc++-6.dll at run time
endif()

add_executable(algoviz main.cpp)
target_link_libraries(algoviz PRIVATE algoviz_core)

# ---- sample plugin as a loadable library
add_library(example_plugin SHARED src/plugin/examples/ExampleDll.cpp)
target_include_directories(example_plugin PRIVATE ${CMAKE_SOURCE_DIR})
set_target_properties(example_plugin PROPERTIES PREFIX "")
if(MINGW)
  target_link_options(example_plugin PRIVATE -static-libstdc++ -static-libgcc)
endif()

# ---- tests (own mini framework, no download needed)
enable_testing()
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tests/*.cpp)
add_executable(algoviz_tests ${TEST_SOURCES})
target_link_libraries(algoviz_tests PRIVATE algoviz_core)
add_dependencies(algoviz_tests example_plugin)
# tests expect to run from the project root and find build/example_plugin.dll|.so there
add_test(NAME all COMMAND algoviz_tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
