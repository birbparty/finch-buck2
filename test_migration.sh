#!/bin/bash

# Simple test script to verify the migration works

set -e

echo "Building the project..."
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

echo "Testing migration on simple-library..."
./build/bin/finch migrate test/projects/simple-library --output-dir test/projects/simple-library

echo "Checking if BUCK file was created..."
if [ -f "test/projects/simple-library/BUCK" ]; then
    echo "✅ BUCK file created!"
    echo "Contents:"
    cat test/projects/simple-library/BUCK
else
    echo "❌ BUCK file not found!"
    exit 1
fi

echo "Comparing with expected output..."
if diff -q test/projects/simple-library/BUCK test/projects/simple-library/BUCK.expected; then
    echo "✅ Generated BUCK file matches expected output!"
else
    echo "❌ Generated BUCK file differs from expected:"
    diff test/projects/simple-library/BUCK test/projects/simple-library/BUCK.expected || true
fi
