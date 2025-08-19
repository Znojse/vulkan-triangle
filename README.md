# game-engine
Game Engine that will be developed with C++, Vulkan and structured by Conan and CMake

# Compilation
```bash
rm -rf build &&
conan install --output-folder=build --profile:a default --build=missing -s build_type=Debug . &&
conan install --output-folder=build --profile:a default --build=missing -s build_type=Release . &&
cmake --workflow --preset=vtAll
```
