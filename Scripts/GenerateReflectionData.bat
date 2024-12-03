@echo off

:: Navigate to the project directory
pushd .. || (
    echo Failed to navigate to the parent directory.
    pause
    exit /b
)

:: Check if premake exists
if not exist "Vendor\Binaries\Premake\Windows\premake5.exe" (
    echo Could not find premake5.exe. Please check the path.
    popd
    pause
    exit /b
)



:: Generate the compile commands
Vendor\Binaries\Premake\Windows\premake5.exe --file=Build.lua ecc || (
    echo Failed to run premake5. Please check the action and paths.
    popd
    pause
    exit /b
)

:: Navigate to the TekEngine-Core source directory
cd Resonance-Core\Source\REON || (
    echo Failed to navigate to Resonance-Core\Source\REON.
    popd
    pause
    exit /b
)

:: Debug: Print the current directory
echo Current directory: %cd%

:: Create a temporary file to hold the header file list
set headerList=temp_headers.txt

:: Find all header files and output them to the temporary file
echo Searching for header files...
(dir /b /s *.h > "..\..\Reflection\%headerList%") || (
    echo No header files found. Creating an empty header file list.
    echo. > "..\..\Reflection\%headerList%"
)

:: Move back to core directory
cd ..\..\Reflection

echo Reflection Directory %cd%

:: Check if the header list file was created
if not exist %headerList% (
    echo Failed to create the header list file.
    popd
    pause
    exit /b
)

:: Check the contents of the header list
echo Contents of %headerList%:
type %headerList%

:: Prepare arguments for ReflectionTool.exe with quotes around each path
setlocal enabledelayedexpansion
set args=
for /f "usebackq delims=" %%i in ("%headerList%") do (
    if not "%%i"=="" (
        set "args=!args! %%i"
    )
)

:: Check if any valid arguments were collected
if "%args%"=="" (
    echo No valid header files found for ReflectionTool.exe.
    del %headerList%
    popd
    pause
    exit /b
)

:: Debug: Output the collected arguments
echo Collected arguments for ReflectionGenerator.exe: %args%

if not exist "..\..\Vendor\ReflectionGenerator\ReflectionGenerator.exe" (
    echo Could not find ReflectionGenerator.exe. Please check the path.
    popd
    pause
    exit /b
)

if exist "..\..\compile_commands.json" (
    echo File found
) else (
    echo File not found
    exit /b 1
)

:: Run the reflection tool with the list of header files
echo Running ReflectionGenerator.exe with the following arguments: %args%

..\..\Vendor\ReflectionGenerator\ReflectionGenerator.exe --compile-commands=..\\..\\compile_commands.json %args%

:: Check if ReflectionTool executed successfully
if errorlevel 1 (
    echo ReflectionGenerator.exe encountered an error.
) else (
    echo ReflectionGenerator.exe completed successfully.
)

:: Clean up the temporary file
del %headerList%

:: Return to the previous directory
popd

:: Pause to keep the command window open for review
echo Finished processing. Press any key to exit.
pause