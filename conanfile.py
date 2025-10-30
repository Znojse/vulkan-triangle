from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.files import copy
import os

class VulkanTriangle(ConanFile):
    name        = "vulkan-triangle"
    settings    = "os", "compiler", "build_type", "arch"
    version     = "0.0.1"
    description = "This is a basic application for rendering a triangle with Vulkan in C++ using Conan and CMake."
    # license = TODO
    author      = "Alexander Olsson (alex.olsson.93@gmail.com)"
    url         = "https://github.com/Znojse/vulkan-triangle"
    package_type = "application"

    def export_sources(self):
        copy(self, "CMakeLists.txt",    self.recipe_folder, self.export_sources_folder)
        copy(self, "CMakePresets.json", self.recipe_folder, self.export_sources_folder)
        copy(self, "*",
             src=os.path.join(self.recipe_folder, "src"),
             dst=os.path.join(self.export_sources_folder, "src"))

    def requirements(self):
        if self.settings.compiler == "msvc" or self.settings.os == "Linux":
            self.requires("glfw/3.4")  # Package is built with MSVC in Windows and does not work with [Windows, mingw] setup.

        self.requires("glm/1.0.1")

        # Requires Vulkan SDK installed build environment.
        # self.requires("vulkan-loader/1.4.313.0")

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.29]")
        if self.settings.compiler != "msvc":
            self.tool_requires("ninja/[>=1.13.1]")

        self.test_requires("catch2/[>=3.9.1]")

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.user_presets_path = 'ConanPresets.json'
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        # err_msg = ("\nThis recipe does not use Conan's build() step."
        #            "\nUse `cmake --workflow` manually after `conan install`."
        #            "\nSee README for examples.")
        # raise ConanInvalidConfiguration(err_msg)

    def layout(self):
        cmake_layout(self, generator=self.conf.get(
            "tools.cmake.cmaketoolchain:generator", default="Ninja Multi-Config"))

    def validate(self):
        if self.settings.os not in ["Linux", "Windows"]:
            raise ConanInvalidConfiguration(f"OS {self.settings.os} is not supported")

        check_min_cppstd(self, "23")

    def package(self):
        cmake = CMake(self)
        cmake.install()
