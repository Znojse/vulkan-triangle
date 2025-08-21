from conan import ConanFile
from conan.tools.cmake import cmake_layout
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps

class VulkanTriangleRecipe(ConanFile):
    name = "Vulkan Triangle"
    settings = "os", "compiler", "build_type", "arch"

    def requirements(self):
        pass

    def build_requirements(self):
        self.requires("ninja/1.13.1")
        self.requires("mingw-builds/15.1.0")
        self.requires("catch2/3.9.1")
        self.requires("cmake/[>=3.29]")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        # cmake.test()
        # self.run("mybuildsystem . --configure")
        # self.run("mybuildsystem . --build")

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.user_presets_path = 'ConanPresets.json'
        tc.generate()

    def layout(self):
        cmake_layout(self)

# Example functions that can be added:
#   - validate, generate ...
