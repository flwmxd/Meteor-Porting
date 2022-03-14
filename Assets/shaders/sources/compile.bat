
@echo off


for %%i in (*.vert *.frag *.comp) do (
    echo "compiling %%i =>>>> %%i.spv" 
    "C:\VulkanSDK\1.2.162.0\Bin\glslc.exe " -o "../spv/%%~i.spv" "%%~i"
)


for /d %%i in (*) do (
    for %%j in (%%i/*.vert %%i/*.frag %%i/*.comp) do (
        echo "compiling %%~i/%%~j =>>>> %%~i/%%~j.spv" 
        "C:\VulkanSDK\1.2.162.0\Bin\glslc.exe " -o "../spv/%%~i/%%~j.spv" "%%~i/%%~j"
    )
)