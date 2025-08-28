from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout

class VulkanTriangle(ConanFile):
    name        = "vulkan-triangle"
    settings    = "os", "compiler", "build_type", "arch"
    version     = "0.0.1"
    description = "This is a basic application for rendering a triangle with Vulkan in C++ using Conan and CMake."
    author      = "Alexander Olsson (alex.olsson.93@gmail.com)"
    url         = "https://github.com/Znojse/vulkan-triangle"

    def requirements(self):
        if self.settings.compiler == "msvc" or self.settings.os == "Linux":
            self.requires("glfw/3.4")  # Package is built with MSVC in Windows and does not work with [Windows, mingw] setup.

        self.requires("glm/1.0.1")
        self.requires("vulkan-loader/1.4.313.0")

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.29]")
        if self.settings.compiler != "msvc":
            self.tool_requires("ninja/[>=1.13.1]")

            if self.settings.os == "Windows":
                self.tool_requires("mingw-builds/[>=15.1.0]")

        self.test_requires("catch2/[>=3.9.1]")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.user_presets_path = 'ConanPresets.json'
        tc.generate()

    def layout(self):
        cmake_layout(self)

    def validate(self):
        pass
