cmake_minimum_required(VERSION 3.29)
project(vulkan-triangle VERSION 0.0.1 LANGUAGES C CXX)

find_package(Catch2 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glm    REQUIRED)

if (CMAKE_C_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(glfw3 REQUIRED)
else()
    include(FetchContent)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
        GIT_SHALLOW TRUE  # Only fetch the latest commit
    )
    FetchContent_MakeAvailable(glfw)
endif()

include(GNUInstallDirs)

add_subdirectory(src)
# add_subdirectory(test)
