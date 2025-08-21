# game-engine
Game Engine that will be developed with C++, Vulkan and structured by Conan and CMake

# Compilation
## Windows
### conan build Release
`conan build . --profile:a ./conan-profiles/win64-clang`

### bash
```bash
clear &&
rm -rf build/* &&
conan install --profile:a ./conan-profiles/win64-clang --build=missing -s build_type=Debug . &&
conan install --profile:a ./conan-profiles/win64-clang --build=missing -s build_type=Release . &&
cmake --workflow --preset=vtAll
```
### Powershell
```powershell
clear ; if (Test-Path -LiteralPath build/) { Remove-Item build/* -Recurse -Force } ; conan install --profile:a ./conan-profiles/win64-clang --build=missing -s build_type=Debug . ; conan install --profile:a ./conan-profiles/win64-clang --build=missing -s build_type=Release . ; cmake --workflow --preset=vtAll
```

# Run
## Windows
```bash
./build/vtDefault/Release/vulkan-triangle_C.exe
# or
./build/vtDefault/Release/vulkan-triangle.exe
```
