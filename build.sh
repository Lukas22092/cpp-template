#!/bin/bash

BUILD_DIR="build"
VENV_DIR="venv" 


if [ "$1" == "clean" ]; then
    echo "Cleaning up build directory..."
    [ -f Logs/logs.log ] && rm Logs/logs.log
    rm -rf "$BUILD_DIR"
fi

if [ "$1" == "analyze" ]; then
    echo "Setting up Python environment for analysis..."
    
    # Create virtual environment if it doesn't exist
    if [ ! -d "$VENV_DIR" ]; then
        echo "Creating virtual environment..."
        python3 -m venv "$VENV_DIR"
    fi
    
    echo "Activating virtual environment and checking dependencies..."
    source "$VENV_DIR"/bin/activate
    
    pip install --upgrade pip --quiet
    pip install matplotlib --quiet
    
    if [ -f "Logs/analyze.py" ]; then
        echo "Running analysis script..."
        python3 Logs/analyze.py
    else
        echo "Error: Logs/analyse.py not found!"
    fi
    
    deactivate
    exit 0 


fi  


if [ ! -d "$BUILD_DIR" ]; then
    echo "Configuring project for the first time..."
    cmake -B "$BUILD_DIR"
fi

echo "Building project..."
cmake --build "$BUILD_DIR" --target template

if [ $? -eq 0 ]; then
    echo "Running project..."
    ./"$BUILD_DIR"/template
else
    echo "Build failed. Skipping execution."
fi 