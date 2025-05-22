cmake_minimum_required(VERSION 3.10)
project(taskmanager)

set(CMAKE_CXX_STANDARD 11)

# Set include directory
include_directories(include)

# Add source files
add_executable(taskmanager
    src/main.cpp
    src/task_manager.cpp
)

# Add warning flags conditionally based on compiler
if (MSVC)
    target_compile_options(taskmanager PRIVATE /W4)
else()
    target_compile_options(taskmanager PRIVATE -Wall -Wextra)
endif()
