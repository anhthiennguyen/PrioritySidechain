#!/usr/bin/env bash
set -e
mkdir -p build
cmake -B build -G Xcode -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug --target PrioritySidechain_VST3 -- -quiet
echo "Build complete. VST3 copied to ~/Library/Audio/Plug-Ins/VST3/"
