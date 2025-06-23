#!/bin/bash
set -e

echo "Ember Core Package Builder"
echo "==========================="
echo ""

# Function to display usage
usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -d, --deb     Build Debian package (.deb)"
    echo "  -r, --rpm     Build RPM package (.rpm)"
    echo "  -a, --all     Build both Debian and RPM packages"
    echo "  -h, --help    Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 --deb      # Build only Debian package"
    echo "  $0 --rpm      # Build only RPM package"
    echo "  $0 --all      # Build both packages"
    echo ""
}

# Parse command line arguments
BUILD_DEB=false
BUILD_RPM=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--deb)
            BUILD_DEB=true
            shift
            ;;
        -r|--rpm)
            BUILD_RPM=true
            shift
            ;;
        -a|--all)
            BUILD_DEB=true
            BUILD_RPM=true
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

# If no options specified, show usage
if [ "$BUILD_DEB" = false ] && [ "$BUILD_RPM" = false ]; then
    echo "Error: No package type specified"
    echo ""
    usage
    exit 1
fi

# Ensure we're in the right directory
if [ ! -f "Makefile" ] || [ ! -d "src" ]; then
    echo "Error: Must be run from the Ember Core root directory"
    exit 1
fi

# Build the application first
echo "Building Ember Core runtime..."
make clean
make all

if [ $? -ne 0 ]; then
    echo "✗ Application build failed!"
    exit 1
fi

echo "✓ Ember Core runtime built successfully!"
echo ""

# Build Debian package if requested
if [ "$BUILD_DEB" = true ]; then
    echo "Building Debian package..."
    ./build-deb.sh
    echo ""
fi

# Build RPM package if requested
if [ "$BUILD_RPM" = true ]; then
    echo "Building RPM package..."
    ./build-rpm.sh
    echo ""
fi

echo "Package building completed!"
echo ""
echo "To test the packages:"
echo "  - Install in a clean environment"
echo "  - Test runtime: ember --version"
echo "  - Test compiler: emberc --help"
echo "  - Test REPL: ember (interactive mode)"