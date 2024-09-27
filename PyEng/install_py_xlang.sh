#!/bin/bash

# Get the Python site-packages path
SITE_PACKAGES=$(python3 -c "import site; print(site.getsitepackages()[-1])")

# Define source directory as the current directory
SOURCE_DIR=$(pwd)

# Print out the site-packages directory
echo "Site-Packages Directory: $SITE_PACKAGES"

# Define destination directory
DEST_DIR="$SITE_PACKAGES/xlang"

# Check if the destination directory exists, and remove it if it does
if [ -d "$DEST_DIR" ]; then
    echo "Removing existing directory: $DEST_DIR"
    rm -rf "$DEST_DIR"
fi

# Create destination directory
echo "Creating directory: $DEST_DIR"
mkdir -p "$DEST_DIR"

# Detect the operating system
OS_TYPE=$(uname)

# macOS
if [ "$OS_TYPE" == "Darwin" ]; then
    # Copy .dylib and .pdb files
    cp "$SOURCE_DIR/pyeng.dylib" "$DEST_DIR"
    cp "$SOURCE_DIR/pyeng.pdb" "$DEST_DIR"
    cp "$SOURCE_DIR/xlang_*.dylib" "$DEST_DIR"
    cp "$SOURCE_DIR/xlang_*.pdb" "$DEST_DIR"

    # Copy pyeng.dylib and rename to xlang.so (macOS uses .so for Python extensions)
    cp "$SOURCE_DIR/pyeng.dylib" "$SITE_PACKAGES/xlang.so"

    # Copy pyeng.pdb and rename to xlang.pdb (optional, if .pdb is available)
    cp "$SOURCE_DIR/pyeng.pdb" "$SITE_PACKAGES/xlang.pdb"

# Linux
elif [ "$OS_TYPE" == "Linux" ]; then
    # Copy .so and .pdb files
    cp "$SOURCE_DIR/pyeng.so" "$DEST_DIR"
    cp "$SOURCE_DIR/pyeng.pdb" "$DEST_DIR"
    cp "$SOURCE_DIR/xlang_*.so" "$DEST_DIR"
    cp "$SOURCE_DIR/xlang_*.pdb" "$DEST_DIR"

    # Copy pyeng.so and rename to xlang.so
    cp "$SOURCE_DIR/pyeng.so" "$SITE_PACKAGES/xlang.so"

    # Copy pyeng.pdb and rename to xlang.pdb (optional, if .pdb is available)
    cp "$SOURCE_DIR/pyeng.pdb" "$SITE_PACKAGES/xlang.pdb"

else
    echo "Unsupported OS: $OS_TYPE"
    exit 1
fi

echo "Installation completed successfully for $OS_TYPE."
