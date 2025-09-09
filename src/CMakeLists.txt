add_executable(vulkan-triangle)

target_sources(vulkan-triangle
    PRIVATE
        main.cpp
        hello_triangle_application.cpp
)

target_sources(vulkan-triangle
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS .
        FILES
        hello_triangle_application.hpp
)

target_link_libraries(vulkan-triangle PRIVATE Vulkan::Vulkan glfw glm::glm)

# Include the shader compilation module.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/shaders/cmake")
include(CompileShaders)
add_shaders(vulkan-triangle-shaders shaders/triangle.vert shaders/triangle.frag)


## TODO
#  - Learn more about install:
#    - install(TARGETS <name>
#      FILE_SET HEADERS
# )
#
# install(RUNTIME_DEPENDENCY_SET lib_deps
#  # Kolla CMAKE videon 2025...
# )


## Vulkan
# set(ENV{VULKAN_SDK} "Path/To/Vulkan/Version/Installation")
# find_package(Vulkan REQUIRED)
# target_link_libraries(target ${Vulkan_LIBRARIES})
