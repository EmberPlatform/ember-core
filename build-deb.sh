#!/bin/bash
set -e

echo "Building Debian package for Ember Core..."

# Check if we have the required tools
if ! command -v debuild &> /dev/null; then
    echo "Error: debuild not found. Please install devscripts:"
    echo "  sudo apt-get install devscripts build-essential"
    exit 1
fi

# Check if we have the required dependencies
if ! command -v dpkg-checkbuilddeps &> /dev/null; then
    echo "Error: dpkg-checkbuilddeps not found. Please install dpkg-dev:"
    echo "  sudo apt-get install dpkg-dev"
    exit 1
fi

# Check build dependencies
echo "Checking build dependencies..."
if ! dpkg-checkbuilddeps debian/control 2>/dev/null; then
    echo "Missing build dependencies. Installing..."
    sudo apt-get update
    sudo apt-get install -y debhelper gcc make libreadline-dev libcurl4-openssl-dev pkg-config
fi

# Clean previous builds
echo "Cleaning previous builds..."
make clean || true
rm -f ../ember-core_*.deb ../ember-core_*.dsc ../ember-core_*.tar.* ../ember-core_*.changes

# Build the package
echo "Building Debian package..."
debuild -us -uc -b

# Check if build was successful
if [ $? -eq 0 ]; then
    echo "✓ Debian package built successfully!"
    echo "Packages created in parent directory:"
    ls -la ../ember-core_*.deb 2>/dev/null || echo "No .deb files found"
    
    echo ""
    echo "To install the package:"
    echo "  sudo dpkg -i ../ember-core_*.deb"
    echo "  sudo apt-get install -f  # Fix any dependency issues"
    echo ""
    echo "To use Ember Core:"
    echo "  ember          # Start interactive REPL"
    echo "  emberc file.ember  # Compile Ember program"
else
    echo "✗ Package build failed!"
    exit 1
fi