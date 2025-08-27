# game-engine
Game Engine that will be developed with C++, Vulkan and structured by Conan and CMake

# Compilation
## Windows
### conan build Release
`conan build . --profile:a ./conan-profiles/win64-clang`

### bash
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
### /etc/wsl.conf
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
