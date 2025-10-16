#!/bin/bash
# Simple build script for luup-agent

set -e  # Exit on error

echo "Building luup-agent..."

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

cd build

# Configure
echo "Configuring with CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DLUUP_BUILD_TESTS=ON \
    -DLUUP_BUILD_EXAMPLES=ON

# Build
echo "Building..."
cmake --build . --config Release -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo "Build complete!"
echo ""
echo "Run tests with: cd build && ctest"
echo "Run examples:"
echo "  ./build/basic_chat models/your-model.gguf"
echo "  ./build/tool_calling models/your-model.gguf"

