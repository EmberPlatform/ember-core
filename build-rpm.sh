#!/bin/bash
set -e

echo "Building RPM package for Ember Core..."

# Check if we have the required tools
if ! command -v rpmbuild &> /dev/null; then
    echo "Error: rpmbuild not found. Please install rpm-build:"
    echo "  # On RHEL/CentOS/Fedora:"
    echo "  sudo yum install rpm-build rpmdevtools"
    echo "  # On openSUSE:"
    echo "  sudo zypper install rpm-build"
    exit 1
fi

# Setup RPM build environment
echo "Setting up RPM build environment..."
rpmdev-setuptree

# Get version from spec file
VERSION=$(grep "^Version:" rpm/ember-core.spec | awk '{print $2}')
RELEASE=$(grep "^Release:" rpm/ember-core.spec | awk '{print $2}' | cut -d'%' -f1)

# Create source tarball
echo "Creating source tarball..."
cd ..
tar --exclude='.git' --exclude='debian' --exclude='*.deb' --exclude='*.rpm' \
    -czf ~/rpmbuild/SOURCES/ember-core-${VERSION}.tar.gz ember-core/
cd ember-core

# Copy spec file
cp rpm/ember-core.spec ~/rpmbuild/SPECS/

# Build the package
echo "Building RPM package..."
rpmbuild -ba ~/rpmbuild/SPECS/ember-core.spec

# Check if build was successful
if [ $? -eq 0 ]; then
    echo "✓ RPM package built successfully!"
    echo "Packages created:"
    ls -la ~/rpmbuild/RPMS/*/ember-core-*.rpm 2>/dev/null || echo "No RPM files found"
    ls -la ~/rpmbuild/SRPMS/ember-core-*.src.rpm 2>/dev/null || echo "No SRPM files found"
    
    echo ""
    echo "To install the package:"
    echo "  # On RHEL/CentOS/Fedora:"
    echo "  sudo yum localinstall ~/rpmbuild/RPMS/*/ember-core-*.rpm"
    echo "  # On openSUSE:"
    echo "  sudo zypper install ~/rpmbuild/RPMS/*/ember-core-*.rpm"
    echo ""
    echo "To use Ember Core:"
    echo "  ember          # Start interactive REPL"
    echo "  emberc file.ember  # Compile Ember program"
else
    echo "✗ Package build failed!"
    exit 1
fi