#!/bin/bash

# Get if we are on mac
if [[ "$OSTYPE" == "darwin"* ]]; then
    # Mac OSX
    echo "Mac OSX"
    clang++ -shared -fPIC --std=c++17 -o libcheckers.so pythonConnector.cpp minimax.cpp board.cpp
else
    # Linux
    echo "Linux"
    g++ -shared -fPIC --std=c++17 -o libcheckers.so pythonConnector.cpp minimax.cpp board.cpp
fi