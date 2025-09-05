WIP

# game-engine
Game Engine that will be developed with C++, Vulkan and structured by Conan and CMake

# Compilation
## bash
```bash
# WSL-Linux
clear &&
rm -rf build/* &&
conan install --profile:a ./conan-profiles/wsl-linux-clang --build=missing -s build_type=Debug . &&
conan install --profile:a ./conan-profiles/wsl-linux-clang --build=missing -s build_type=Release . &&
cmake --workflow --preset=vtAll
```
```bash
# Windows MSVC
clear &&
rm -rf build/* &&
conan install --profile:a ./conan-profiles/win64-msvc --build=missing -s build_type=Debug . &&
conan install --profile:a ./conan-profiles/win64-msvc --build=missing -s build_type=Release . &&
cmake --workflow --preset=vtMSVCAll
```

### Cross-compilation Linux -> Windows (Does not work with Vulkan...)
```bash
clear &&
rm -rf build/* &&
conan install --profile:b ./conan-profiles/wsl-linux-gcc --profile:h ./conan-profiles/wsl-win64-gcc --build=missing -s build_type=Debug . &&
conan install --profile:b ./conan-profiles/wsl-linux-gcc --profile:h ./conan-profiles/wsl-win64-gcc --build=missing -s build_type=Release . &&
cmake --workflow --preset=vtAll
```

### Powershell
```powershell
# Windows MSVC
clear ; if (Test-Path -LiteralPath build/) { Remove-Item build/* -Recurse -Force } ; conan install --profile:a ./conan-profiles/win64-msvc --build=missing -s build_type=Debug . ; conan install --profile:a ./conan-profiles/win64-msvc --build=missing -s build_type=Release . ; cmake --workflow --preset=vtMSVCAll
```

# Run
## Windows
```bash
./build/vtDefault/Release/vulkan-triangle_C.exe
# or
./build/vtDefault/Release/vulkan-triangle.exe
```

# Environment
## Requirements
- Python3
    - Pip
    - Venv
- Conan
- CMake
## Setup WSL Debian
Need to install mesa-vulkan-drivers on Linux to work.
### /etc/wsl.conf
### Vulkan Layers
source ~/vulkan/1.4.321.1/setup-env.sh
```
[boot]
systemd=true

[automount]
options = "metadata"
```
Activate python virtual environment.
## Setup Windows
- Install Python3
- Install Conan
- Install CMake

# Format
`clang-format -i src/*.cpp src/*.hpp`

# Valgrind
```bash
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-out.txt ./build/vulkan-triangle/src/Release/vulkan-triangle
```
