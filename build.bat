@echo off
REM Simple build script for luup-agent on Windows

echo Building luup-agent...

REM Create build directory if it doesn't exist
if not exist build mkdir build

cd build

REM Configure
echo Configuring with CMake...
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DLUUP_BUILD_TESTS=ON ^
    -DLUUP_BUILD_EXAMPLES=ON

if %errorlevel% neq 0 (
    echo CMake configuration failed!
    exit /b %errorlevel%
)

REM Build
echo Building...
cmake --build . --config Release

if %errorlevel% neq 0 (
    echo Build failed!
    exit /b %errorlevel%
)

echo.
echo Build complete!
echo.
echo Run tests with: cd build ^&^& ctest -C Release
echo Run examples:
echo   build\Release\basic_chat.exe models\your-model.gguf
echo   build\Release\tool_calling.exe models\your-model.gguf

cd ..

