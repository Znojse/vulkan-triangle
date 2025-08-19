from conan import ConanFile
from conan.tools.cmake import cmake_layout

class VulkanTriangleRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"
    # generators = "CMakeToolchain", "CMakeDeps"

    def requirements(self):
        self.requires("catch2/3.9.1")

    def build_requirements(self):
        pass

    def layout(self):
        cmake_layout(self)

# import os

# from conan import ConanFile
# from conan.tools.cmake import cmake_layout
# from conan.errors import ConanInvalidConfiguration
# # from conan.tools.files import copy
# from conan.tools.cmake import CMake

# class CompressorRecipe(ConanFile):
#     settings = "os", "compiler", "build_type", "arch"
#     generators = "CMakeToolchain", "CMakeDeps"

#     def requirements(self):
#         self.requires("zlib/1.3.1")

#         # Add base64 dependency for Windows
#         if self.settings.os == "Windows":
#             self.requires("base64/0.4.0")

#     def build_requirements(self):
#         # Use the system's CMake for Windows
#         if self.settings.os != "Windows":
#             self.tool_requires("cmake/3.27.9")

#     def layout(self):
#         cmake_layout(self)

#     def validate(self):
#         if self.settings.os == "Macos" and self.settings.arch == "armv8":
#             raise ConanInvalidConfiguration("ARM V8 not supported in MacOS")

#     # def generate(self):
#     #     # Copy all resources from the dependency's resource directory
#     #     # to the "assets" folder in the source directory of your project
#     #     dep = self.dependencies["dep_name"]
#     #     copy(self, "*", dep.cpp_info.resdirs[0], os.path.join(self.source_folder, "assets"))

#     def build(self):
#         cmake = CMake(self)
#         cmake.configure()
#         cmake.build()
