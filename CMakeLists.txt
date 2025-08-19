cmake_minimum_required(VERSION 3.15)
project(vulkan-triangle C CXX)

find_package(Catch2 REQUIRED)

message("Building with CMake version: ${CMAKE_VERSION}")

add_executable(${PROJECT_NAME} src/main.c)
