#!/bin/bash
# Quick script to build and run the interactive CLI for testing

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Building luup-agent interactive CLI...${NC}"

# Save the starting directory
START_DIR="$(pwd)"

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir -p build
fi

cd build

# Configure if needed
if [ ! -f "Makefile" ] && [ ! -f "build.ninja" ]; then
    echo "Configuring CMake..."
    cmake .. -DLUUP_BUILD_EXAMPLES=ON
fi

# Build
echo "Building..."
cmake --build . --target interactive_cli --config Release -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Build successful!${NC}"
    echo ""
    
    # Check if model path is provided
    if [ $# -eq 0 ]; then
        echo -e "${YELLOW}Usage:${NC} $0 <path-to-model.gguf> [options]"
        echo ""
        echo "Examples:"
        echo "  $0 models/qwen-0.5b.gguf"
        echo "  $0 models/tiny-llama.gguf --temp 0.9"
        echo "  $0 models/phi-2.gguf --no-tools"
        echo ""
        echo "The interactive_cli binary is available at:"
        echo "  $(pwd)/examples/interactive_cli"
        exit 1
    fi
    
    # Convert first argument (model path) to absolute path if it's relative
    MODEL_PATH="$1"
    shift  # Remove first argument
    
    # Check if path is relative and convert to absolute
    if [[ ! "$MODEL_PATH" = /* ]]; then
        MODEL_PATH="${START_DIR}/${MODEL_PATH}"
    fi
    
    # Run the CLI with absolute model path and remaining arguments
    echo -e "${GREEN}Starting interactive CLI...${NC}"
    echo ""
    ./examples/interactive_cli "$MODEL_PATH" "$@"
else
    echo -e "${RED}✗ Build failed!${NC}"
    exit 1
fi

