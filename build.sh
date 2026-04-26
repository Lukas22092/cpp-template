#!/bin/bash

set -e
clang-format -i src/*.cpp include/*.hpp

BUILD_DIR="build"

if [ "$1" == "clean" ]; then
    echo "Cleaning up old build files..."
    rm -rf $BUILD_DIR
fi

# Create the build directory if it doesnt exist
if [ ! -d "$BUILD_DIR" ]; then
    mkdir $BUILD_DIR
fi

cd $BUILD_DIR

echo "Configuring project..."
cmake ..

echo "Starting build..."
cmake --build .

echo "------------------------"
echo "Build complete!"

echo "Running project..."
./template