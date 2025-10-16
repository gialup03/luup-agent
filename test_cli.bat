@echo off
REM Quick script to build and run the interactive CLI for testing on Windows

echo Building luup-agent interactive CLI...

REM Create build directory if it doesn't exist
if not exist "build" (
    echo Creating build directory...
    mkdir build
)

cd build

REM Configure if needed
if not exist "CMakeCache.txt" (
    echo Configuring CMake...
    cmake .. -DLUUP_BUILD_EXAMPLES=ON
)

REM Build
echo Building...
cmake --build . --target interactive_cli --config Release

if %ERRORLEVEL% EQU 0 (
    echo Build successful!
    echo.
    
    REM Check if model path is provided
    if "%~1"=="" (
        echo Usage: %0 ^<path-to-model.gguf^> [options]
        echo.
        echo Examples:
        echo   %0 models\qwen-0.5b.gguf
        echo   %0 models\tiny-llama.gguf --temp 0.9
        echo   %0 models\phi-2.gguf --no-tools
        echo.
        echo The interactive_cli binary is available at:
        echo   %CD%\examples\Release\interactive_cli.exe
        exit /b 1
    )
    
    REM Run the CLI
    echo Starting interactive CLI...
    echo.
    examples\Release\interactive_cli.exe %*
) else (
    echo Build failed!
    exit /b 1
)

