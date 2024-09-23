@echo off
if not exist build\win_x64_vulkan (
    mkdir build\win_x64_vulkan
)
cd build\win_x64_vulkan
cmake ..\..\  -DArch=x64 -DTarget=Windows -DBuildType=Editor -DCMAKE_BUILD_TYPE=Debug -DMAPLE_OPENGL=OFF -DMAPLE_VULKAN=ON -DDISABLE_FILESYSTEM=ON -DBUILD_STATIC=ON -DDISABLE_TESTS=OFF
cd ..\..\
pause