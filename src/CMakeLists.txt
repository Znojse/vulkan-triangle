# Include the shader compilation module.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/shaders/cmake")
include(CompileShaders)

# Compile the shaders.
set(SHADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets/shaders")
file(MAKE_DIRECTORY "${SHADER_DIR}")
add_shaders(vulkan-triangle-shaders shaders/triangle.vert shaders/triangle.frag)

# Generate a shader-config.hpp with installation paths.
set(SHADER_INSTALL_DIR "${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME}/shaders")
set(GENERATED_FILES_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${GENERATED_FILES_DIR}")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader-config.hpp.in
    ${GENERATED_FILES_DIR}/shader-config.hpp
    @ONLY
)


# Add the executable.
add_executable(vulkan-triangle)

target_sources(vulkan-triangle
    PRIVATE
        main.cpp
        hello_triangle_application.cpp
)

target_sources(vulkan-triangle
    PRIVATE
        FILE_SET HEADERS
        BASE_DIRS .
        FILES
            hello_triangle_application.hpp
            vulkan_validation.hpp
            utilities.hpp
)

set_source_files_properties("${GENERATED_FILES_DIR}/shader-config.hpp" PROPERTIES GENERATED TRUE)
target_include_directories(vulkan-triangle
    PRIVATE
        ${GENERATED_FILES_DIR}
)

target_link_libraries(vulkan-triangle PRIVATE Vulkan::Vulkan glfw glm::glm)


# Install for delivery.
install(
    DIRECTORY
        ${SHADER_DIR}/
        DESTINATION
        ${SHADER_INSTALL_DIR}
    FILES_MATCHING PATTERN "*.spv"
)

install(
    TARGETS vulkan-triangle
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    FILE_SET HEADERS
)

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
