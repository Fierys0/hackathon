#!/bin/bash
# Build Sentinel for Windows (x64) using MinGW cross-compiler
set -e

BUILD_DIR="build_windows"

echo "=== Sentinel - Windows Build ==="
echo "Creating build directory: $BUILD_DIR"
mkdir -p "$BUILD_DIR"

echo "Running CMake with MinGW toolchain..."
cmake -S . -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE=cmake/windows-mingw-toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DMINGW_PREFIX=x86_64-w64-mingw32

echo "Building..."
cmake --build "$BUILD_DIR" --parallel $(nproc)

echo ""
echo "=== Build complete! ==="
echo "Output files are in: $BUILD_DIR/hackathon/"
echo ""
echo "To run on Windows, copy the entire '$BUILD_DIR/hackathon/' folder."
echo "Make sure config/api_key.txt is placed alongside the executable."
