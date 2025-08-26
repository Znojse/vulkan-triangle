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

# add_executable(${PROJECT_NAME}_C src/main.c)
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan glfw glm::glm)


## Nice stuff later in subdirs.
# target_sources(<libname>
#     PRIVATE
#         lib/foo.cpp
#         lib/bar.cpp
# )
#
# target_sources(<libname>
#     PUBLIC
#         FILE_SET HEADERS
#         BASE_DIRS .
#         FILES
#             lib/foo.hpp
#             lib/bar.hpp
# )
#
# install(TARGETS <name>
#     FILE_SET HEADERS
# )
#
# install(RUNTIME_DEPENDENCY_SET lib_deps
#  # Kolla CMAKE videon 2025...
# )


## Vulkan
# set(ENV{VULKAN_SDK} "Path/To/Vulkan/Version/Installation")
# find_package(Vulkan REQUIRED)
# target_link_libraries(target ${Vulkan_LIBRARIES})
