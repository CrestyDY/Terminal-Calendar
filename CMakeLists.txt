cmake_minimum_required(VERSION 3.10)
project(taskmanager)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_executable(taskmanager
    src/main.cpp
    src/task_manager.cpp
)

if (MSVC)
    target_compile_options(taskmanager PRIVATE /W4 /utf-8)
else()
    target_compile_options(taskmanager PRIVATE -Wall -Wextra)
endif()

# Add these two lines to use nlohmann-json with vcpkg
find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(taskmanager PRIVATE nlohmann_json::nlohmann_json)


configure_file(${CMAKE_SOURCE_DIR}/config.json ${CMAKE_BINARY_DIR}/config.json COPYONLY)

if(CMAKE_CONFIGURATION_TYPES)
    foreach(config ${CMAKE_CONFIGURATION_TYPES})
        configure_file(${CMAKE_SOURCE_DIR}/config.json ${CMAKE_BINARY_DIR}/${config}/config.json COPYONLY)
    endforeach()
endif()
