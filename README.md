# Table of Contents
1. [Vulkan Triangle](#vulkan-triangle)
    1. [Project Structure](#project-structure)
2. [Prerequisites](#prerequisites)
    1. [Setup Environment](#setup-environment)
        1. [Linux](#linux)
        2. [Windows](#windows)
3. [Compilation](#compilation)
    1. [Conan Profiles](#conan-profiles)
    2. [Workflow Presets](#workflow-presets)
    3. [Instructions](#instructions)
        1. [Compile Examples](#compile-examples)
            1. [Bash](#bash)
            2. [Powershell](#powershell)
4. [Run](#run)
5. [Development](#development)
    1. [Formatting](#formatting)
    2. [Debugging](#debugging)
        1. [Valgrind](#valgrind)
    3. [Validation Layers](#validation-layers)

# Vulkan Triangle
An application generating a RGB triangle within a window using Vulkan and following this [vulkan-tutorial](https://vulkan-tutorial.com/).
The project is developed in C++ using the Vulkan API and setting up the the project structure with Conan, CMake and CMakePresets.

## Project Structure
```
vulkan-triangle
|    .clang-format
|    .gitignore
|    CMakeLists.txt                        # Create libraries and executables.
|    CMakePresets.json                     # Presets for configurations, build, test and workflow.
|    conanfile.py                          # Define build instructions and manages dependencies.
|    README.txt
|
|----.vscode
|        .c_cpp_properties.json            # Template to activate intelliSense for Vulkan in Windows and Linux environment.
|
|----conan-profiles                        # Profiles for different target environments.
|        linux64-clang
|        linux64-gcc
|        linux64-to-win64-gcc
|        win64-msvc
|
|----src
|    |    CMakeLists.txt
|    |    hello_triangle_application.cpp
|    |    hello_triangle_application.hpp
|    |    main.cpp
|    |
|    ----shaders                           # Shaders determine how surfaces and objects appear in a digital scene.
|    |    |    trinagle.frag
|    |    |    trinagle.vert
|    |    ----cmake
|    |    |       CompileShaders.cmake     # CMake module to compile the shaders during application compilation.
```

# Prerequisites
The projects assumes that the [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) is pre-installed in the development environment. It is possible to compile and run from both Windows using MSVC or from Linux using either clang or gcc, see available conan profiles [here](#conan-profiles).

## Setup Environment
### Linux
**WIP**
- Python3
    - Pip
    - Venv
- Conan
- CMake
#### Setup WSL Debian
Need to install mesa-vulkan-drivers on Linux to work.
#### /etc/wsl.conf
#### Vulkan Layers
source ~/vulkan/1.4.321.1/setup-env.sh
```
[boot]
systemd=true

[automount]
options = "metadata"
```
Activate python virtual environment.

### Windows
**WIP**
- Install Python3
- Install Conan
- Install CMake

# Compilation
## Conan Profiles
Used to make it easier to build for different targets with different compilers.
* linux64-clang
* linux64-gcc
* linux64-to-win64-gcc
    * Not currently supported.
    * Due to being complex with Vulkan its not currently possible to use the `linux64-to-win64-gcc` profile for cross-compilation.
* win64-clang
    * Compiles but does not run as expected, GLFW window is not opened.
* win64-msvc

## Workflow Presets
Used to simplify and standardize the process of building and testing the project.
* vtDebug
* vtRelease
* vtMSVCDebug
* vtMSVCRelease
* vtAll
* vtMSVCAll

## Instructions
1. Run `conan install` with prefered profile for both `Debug` and `Release`.
2. Run cmake with preferred workflow preset.

### Compile Examples
#### Bash
```bash
# Linux
clear &&
rm -rf build/* &&
conan install --profile:a ./conan-profiles/linux64-clang --build=missing -s build_type=Debug . &&
conan install --profile:a ./conan-profiles/linux64-clang --build=missing -s build_type=Release . &&
cmake --workflow --preset=vtAll
```
```bash
# MSVC
clear &&
rm -rf build/* &&
conan install --profile:a ./conan-profiles/win64-msvc --build=missing -s build_type=Debug . &&
conan install --profile:a ./conan-profiles/win64-msvc --build=missing -s build_type=Release . &&
cmake --workflow --preset=vtMSVCAll
```

```bash
# Cross-compilation Linux -> Windows (Does not work with Vulkan...)
clear &&
rm -rf build/* &&
conan install --profile:b ./conan-profiles/linux64-gcc --profile:h ./conan-profiles/linux64-to-win64-gcc --build=missing -s build_type=Debug . &&
conan install --profile:b ./conan-profiles/linux64-gcc --profile:h ./conan-profiles/linux64-to-win64-gcc --build=missing -s build_type=Release . &&
cmake --workflow --preset=vtAll
```

#### Powershell
```powershell
# Windows MSVC
clear ; if (Test-Path -LiteralPath build/) { Remove-Item build/* -Recurse -Force } ; conan install --profile:a ./conan-profiles/win64-msvc --build=missing -s build_type=Debug . ; conan install --profile:a ./conan-profiles/win64-msvc --build=missing -s build_type=Release . ; cmake --workflow --preset=vtMSVCAll
```

# Run
```bash
# Linux
./build/vulkan-triangle/src/Debug/vulkan-triangle
./build/vulkan-triangle/src/Release/vulkan-triangle
# Windows
./build/vulkan-triangle/src/Debug/vulkan-triangle.exe
./build/vulkan-triangle/src/Release/vulkan-triangle.exe
```

# Development
**WIP**
## Formatting
`clang-format -i src/*.cpp src/*.hpp`
## Debugging
### Valgrind
```bash
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-out.txt ./build/vulkan-triangle/src/Release/vulkan-triangle
```

## Validation Layers
