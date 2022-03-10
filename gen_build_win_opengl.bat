@echo off
if not exist build\win_x64_game_opengl (
    mkdir build\win_x64_game_opengl
)
cd build\win_x64_game_opengl
cmake ..\..\ -G "Visual Studio 16 2019" -DArch=x64 -DTarget=Windows -DMAPLE_OPENGL=ON -DMAPLE_VULKAN=OFF 
cd ..\..\
pause