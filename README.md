# Table of Contents
1. [Vulkan Triangle](#vulkan-triangle)
    1. [Project Structure](#project-structure)
2. [Prerequisites](#prerequisites)
    1. [Setup Environment](#setup-environment)
        1. [Linux](#linux)
            * [Debian](#debian)
        2. [Windows](#windows)
            * [MSYS2](#msys2)
3. [Compilation](#compilation)
    1. [Conan Profiles](#conan-profiles)
    2. [Workflow Presets](#workflow-presets)
    3. [Instructions](#instructions)
        1. [Compile Examples](#compile-examples)
            1. [Bash](#bash)
            2. [Powershell](#powershell)
        2. [Run Examples](#run-examples)
4. [Development](#development)
    1. [Formatting](#formatting)
    2. [Debugging](#debugging)
        1. [Valgrind](#valgrind)
        2. [Validation Layers](#validation-layers)

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
|        .c_cpp_properties.json.template   # Template to activate intelliSense for Vulkan in Windows and Linux environment.
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
#### Debian
1. Download and extract the Vulkan SDK:
    1. Download the `tar.xz` for Linux from [LunarG](https://www.lunarg.com/vulkan-sdk/).
    2. Create a vulkan folder in the home directory:
        * `mkdir ~/vulkan`.
    3. Extract the `tar.xz` into the vulkan directory:
        1. `sudo apt install xz-utils`
        2. `sudo tar -xvf <path>/vulkansdk-linux-x86_64-<1.x.yy.z>.tar.xz -C ~/vulkan`
2. [Setup](https://vulkan.lunarg.com/doc/sdk/1.4.321.1/linux/getting_started.html) Vulkan environment variables:
    * Source the vulkan `setup-env` to the current shell, or into e.g. the `.profile` file for every session.
        * `source ~/vulkan/1.x.yy.z/setup-env.sh`
    * Alternatively setup the paths by setting these environment variables manually:
        * `export VULKAN_SDK=~/vulkan/1.x.yy.z/x86_64` \
        `export PATH=$VULKAN_SDK/bin:$PATH` \
        `export LD_LIBRARY_PATH=$VULKAN_SDK/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}` \
        `export VK_ADD_LAYER_PATH=$VULKAN_SDK/share/vulkan/explicit_layer.d`
1. Install Python3:
    1. `sudo apt update`
    2. `sudo apt upgrade`
    3. `sudo apt install python3`
    4. `sudo apt install python-is-python3`
    5. `sudo apt install python3.13-venv`
2. Activate the Python virtual environment:
    1. `python -m venv <path>/.venv`
    2. `source <path>/.venv/bin/activate`
4. Install dependencies within `.venv`:
    1. `python -m pip install --upgrade pip`
    2. `pip install conan`
    3. `pip install cmake`
5. Update and install project dependencies:
    1. `sudo apt install pkg-config`
    2. `sudo apt install cmake`
    3. `sudo apt install clang`
    4. `sudo apt install ninja-build`
    5. `sudo apt install mesa-vulkan-drivers`

### Windows
1. Download and install the Vulkan SDK installer from [LunarG](https://www.lunarg.com/vulkan-sdk/).
    * Run the installer and follow its instructions.
2. Install the MSVC compiler, e.g. through Visual Studio.
3. Install Python3
    1. Create the Python virtual environment:
        * `python -m venv <path>/.venv`
    2. Activate the Python virtual environment:
        * Powershell: `<path>/.venv/Scripts/Activate.ps1`
        * Bash: `source <path>/.venv/Scripts/activate`
4. Install dependencies within `.venv`:
    1. `python -m pip install --upgrade pip`
    2. `pip install conan`
    3. `pip install cmake`

#### MSYS2
To be able to use other compilers other than `MSVC`, e.g. by using the `win64-clang` conan profile, MSYS2 is required.
1. Download and run the installer from [msys2](https://www.msys2.org/).
    * The recommended settings should suffice.
2. In the MSYS2 terminal, install the MinGW-w64 toolchain.
    * `pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain`
3. Add the path of the MinGW-w64 bin folder to the Windows `PATH` environment.
    * Example of MinGW-w64 path: `C:\msys64\ucrt64\bin`.
    * A step by step guide can be found [here](https://code.visualstudio.com/docs/cpp/config-mingw).

# Compilation
## Conan Profiles
Used to make it easier to build for different targets with different compilers.
* linux64-clang
* linux64-gcc
* linux64-to-win64-gcc
    * Not currently supported.
    * Due to being complex with Vulkan its not currently possible to use the `linux64-to-win64-gcc` profile for cross-compilation.
* win64-clang
    * Requires MSYS2/MinGW, see [MSYS2](#msys2).
* win64-msvc

## Workflow Presets
Used to simplify and standardize the process of building and testing the project.
* vtDefaultDebug
* vtDefaultRelease
* vtMSVCDebug
* vtMSVCRelease
* vtDefaultAll
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
cmake --workflow --preset=vtDefaultAll
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
cmake --workflow --preset=vtDefaultAll
```

#### Powershell
```powershell
# Windows MSVC
clear ; if (Test-Path -LiteralPath build/) { Remove-Item build/* -Recurse -Force } ; `
conan install --profile:a ./conan-profiles/win64-msvc --build=missing -s build_type=Debug . ; `
conan install --profile:a ./conan-profiles/win64-msvc --build=missing -s build_type=Release . ; `
cmake --workflow --preset=vtMSVCAll
```
```powershell
# Windows Clang
# Requires MSYS2, see the MSYS2 section under Prerequisites.
clear ; if (Test-Path -LiteralPath build/) { Remove-Item build/* -Recurse -Force } ; `
conan install --profile:a ./conan-profiles/win64-clang --build=missing -s build_type=Debug . ; `
conan install --profile:a ./conan-profiles/win64-clang --build=missing -s build_type=Release . ; `
cmake --workflow --preset=vtDefaultAll
```

### Run Examples
```bash
# Linux
./build/vulkan-triangle/src/Debug/vulkan-triangle
./build/vulkan-triangle/src/Release/vulkan-triangle
# Windows
./build/vulkan-triangle/src/Debug/vulkan-triangle.exe
./build/vulkan-triangle/src/Release/vulkan-triangle.exe
```

# Development
Tools used to simplify the development.

## Formatting
`clang-format` is used to adhere to the preferred style in the project by using the `.clang-format` configuration file.

1. Install clang-format:
    * Linux: `sudo apt install clang-format`
2. Run the command:
    * `clang-format -i src/*.cpp src/*.hpp`

## Debugging
### Valgrind
Valgrind is an instrumentation framework used for debugging and profiling programs, primarily to detect memory-related issues like memory leaks, undefined memory usage, and memory corruption.

```bash
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-out.txt ./build/vulkan-triangle/src/Release/vulkan-triangle
```

### Validation Layers
When building for `Debug` this project adds the Vulkan Validation Layers while they are skipped when building for `Release`.

In Vulkan, validation layers are a set of debugging and error-checking tools that help developers identify mistakes or improper usage of Vulkan API calls during development. They don't affect the performance of the application in production but are invaluable during the development and debugging stages.

See section [Linux Debian](#debian) and [Windows](#windows) for setup instructions and the [Vulkan Docs](https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/00_Setup/02_Validation_layers.html) for more information.
