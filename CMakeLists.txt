cmake_minimum_required(VERSION 3.29)
project(vulkan-triangle VERSION 0.0.1 LANGUAGES C CXX)

find_package(Catch2 REQUIRED)

include(GNUInstallDirs)

message("Building with CMake version: ${CMAKE_VERSION}")
message("CMAKE SOURCE DIR: ${CMAKE_SOURCE_DIR}")

add_executable(${PROJECT_NAME} src/main.cpp)
add_executable(${PROJECT_NAME}_C src/main.c)


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
