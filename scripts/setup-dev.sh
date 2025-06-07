#!/bin/bash

# Development environment setup script for finch-buck2
# This script sets up the development environment with all necessary tools

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to check minimum version
check_version() {
    local cmd="$1"
    local min_version="$2"
    local current_version

    case "$cmd" in
        cmake)
            current_version=$(cmake --version | head -n1 | cut -d' ' -f3)
            ;;
        python3)
            current_version=$(python3 --version | cut -d' ' -f2)
            ;;
        clang++)
            current_version=$(clang++ --version | head -n1 | grep -o '[0-9]\+\.[0-9]\+' | head -n1)
            ;;
        g++)
            current_version=$(g++ --version | head -n1 | grep -o '[0-9]\+\.[0-9]\+' | head -n1)
            ;;
        *)
            return 0
            ;;
    esac

    if command_exists python3; then
        python3 -c "
import sys
from packaging import version
if version.parse('$current_version') >= version.parse('$min_version'):
    sys.exit(0)
else:
    sys.exit(1)
" 2>/dev/null || {
        # Fallback to basic version comparison if packaging is not available
        if [[ "$current_version" == "$min_version" ]] || [[ "$current_version" > "$min_version" ]]; then
            return 0
        else
            return 1
        fi
    }
    else
        return 0
    fi
}

print_status "Setting up finch-buck2 development environment..."

# Check operating system
OS=$(uname -s)
print_status "Detected OS: $OS"

# Check required tools
print_status "Checking required tools..."

# Check CMake
if command_exists cmake; then
    if check_version cmake "3.20.0"; then
        print_success "CMake $(cmake --version | head -n1 | cut -d' ' -f3) found"
    else
        print_warning "CMake version is too old, please upgrade to 3.20.0 or newer"
    fi
else
    print_error "CMake not found. Please install CMake 3.20.0 or newer"
    exit 1
fi

# Check C++ compiler
if command_exists clang++; then
    if check_version clang++ "14.0"; then
        print_success "Clang++ $(clang++ --version | head -n1 | grep -o '[0-9]\+\.[0-9]\+' | head -n1) found"
        PREFERRED_CXX="clang++"
        PREFERRED_CC="clang"
    else
        print_warning "Clang++ version is too old"
    fi
elif command_exists g++; then
    if check_version g++ "11.0"; then
        print_success "G++ $(g++ --version | head -n1 | grep -o '[0-9]\+\.[0-9]\+' | head -n1) found"
        PREFERRED_CXX="g++"
        PREFERRED_CC="gcc"
    else
        print_warning "G++ version is too old"
    fi
else
    print_error "No suitable C++ compiler found. Please install Clang 14+ or GCC 11+"
    exit 1
fi

# Check Python
if command_exists python3; then
    if check_version python3 "3.8.0"; then
        print_success "Python $(python3 --version | cut -d' ' -f2) found"
    else
        print_warning "Python version is too old, some tools may not work"
    fi
else
    print_error "Python 3 not found. Please install Python 3.8 or newer"
    exit 1
fi

# Check Git
if command_exists git; then
    print_success "Git $(git --version | cut -d' ' -f3) found"
else
    print_error "Git not found. Please install Git"
    exit 1
fi

# Install pre-commit if not present
print_status "Setting up pre-commit hooks..."
if command_exists pre-commit; then
    print_success "pre-commit is already installed"
else
    print_status "Installing pre-commit..."
    if command_exists pip3; then
        pip3 install --user pre-commit
    elif command_exists pip; then
        pip install --user pre-commit
    else
        print_error "pip not found. Please install pre-commit manually: pip3 install pre-commit"
        exit 1
    fi
fi

# Install pre-commit hooks
if [ -f .pre-commit-config.yaml ]; then
    print_status "Installing pre-commit hooks..."
    pre-commit install
    print_success "Pre-commit hooks installed"
else
    print_warning ".pre-commit-config.yaml not found, skipping pre-commit setup"
fi

# Create build directory
print_status "Setting up build directory..."
if [ ! -d "build" ]; then
    mkdir build
    print_success "Build directory created"
else
    print_success "Build directory already exists"
fi

# Configure CMake
print_status "Configuring CMake..."
cd build

CMAKE_ARGS=(
    "-DCMAKE_BUILD_TYPE=Debug"
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    "-DFINCH_BUILD_TESTS=ON"
    "-DFINCH_ENABLE_WARNINGS=ON"
)

if [[ -n "${PREFERRED_CC:-}" ]]; then
    CMAKE_ARGS+=("-DCMAKE_C_COMPILER=$PREFERRED_CC")
fi

if [[ -n "${PREFERRED_CXX:-}" ]]; then
    CMAKE_ARGS+=("-DCMAKE_CXX_COMPILER=$PREFERRED_CXX")
fi

# Use Ninja if available
if command_exists ninja; then
    CMAKE_ARGS+=("-G" "Ninja")
    print_status "Using Ninja build system"
elif command_exists ninja-build; then
    CMAKE_ARGS+=("-G" "Ninja")
    print_status "Using Ninja build system"
fi

cmake .. "${CMAKE_ARGS[@]}"
cd ..

print_success "CMake configuration complete"

# Build the project to verify everything works
print_status "Building project to verify setup..."
if command_exists ninja && [ -f "build/build.ninja" ]; then
    cmake --build build --parallel
elif [ -f "build/Makefile" ]; then
    cmake --build build --parallel
else
    cmake --build build
fi

print_success "Build successful"

# Run tests if they exist
if [ -f "build/test/dummy_test" ] || [ -f "build/test/dummy_test.exe" ]; then
    print_status "Running tests..."
    cd build
    ctest --output-on-failure
    cd ..
    print_success "Tests passed"
fi

# Create symlink to compile_commands.json for IDE support
if [ -f "build/compile_commands.json" ] && [ ! -f "compile_commands.json" ]; then
    ln -s build/compile_commands.json compile_commands.json
    print_success "Created compile_commands.json symlink for IDE support"
fi

# Print final status
echo
print_success "Development environment setup complete!"
echo
print_status "Next steps:"
echo "  1. Open the project in your favorite IDE"
echo "  2. Start coding! The pre-commit hooks will ensure code quality"
echo "  3. Run 'cmake --build build' to build the project"
echo "  4. Run 'cd build && ctest' to run tests"
echo
print_status "Useful commands:"
echo "  - Format code: find src include -name '*.cpp' -o -name '*.hpp' | xargs clang-format -i"
echo "  - Run static analysis: clang-tidy src/*.cpp -p build"
echo "  - Run pre-commit checks: pre-commit run --all-files"
echo
print_status "Happy coding!"
