cmake_minimum_required(VERSION 3.10)
project(taskmanager)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_executable(taskmanager
    src/main.cpp
    src/task_manager.cpp
)

if (MSVC)
    target_compile_options(taskmanager PRIVATE /W4)
else()
    target_compile_options(taskmanager PRIVATE -Wall -Wextra)
endif()
