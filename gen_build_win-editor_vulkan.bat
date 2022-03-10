@echo off
if not exist build\win_x64_vulkan (
    mkdir build\win_x64_vulkan
)
cd build\win_x64_vulkan
cmake ..\..\  -G "Visual Studio 16 2019" -DArch=x64 -DTarget=Windows -DBuildType=Editor -DCMAKE_BUILD_TYPE=Debug -DMAPLE_OPENGL=OFF -DMAPLE_VULKAN=ON
cd ..\..\
pause