#!/bin/bash
# build.sh - Linux/Mac build script for pfodWeb
# Builds standalone HTML files with inlined JavaScript
# (c)2025 Forward Computing and Control Pty. Ltd.

echo "========================================"
echo "  pfodWeb Bundle Builder (Linux/Mac)"
echo "========================================"
echo ""

# Check if Node.js is installed
if ! command -v node &> /dev/null; then
    echo "ERROR: Node.js is not installed or not in PATH"
    echo "Please install Node.js from https://nodejs.org/"
    echo ""
    exit 1
fi

# Check if build script exists
if [ ! -f build-bundle.js ]; then
    echo "ERROR: build-bundle.js not found"
    echo "Please ensure you are running this from the pfodWebServer directory"
    echo ""
    exit 1
fi

echo "Building standalone HTML files..."
echo ""

# Run the build script
node build-bundle.js

if [ $? -eq 0 ]; then
    echo ""
    echo "========================================"
    echo "  Build Successful!"
    echo "========================================"
    echo ""
    echo "Output files are in the dist/ folder:"
    echo "  - index.html"
    echo "  - pfodWeb.html"
    echo "  - pfodWebDebug.html"
    echo ""
    echo "To test:"
    echo "  1. cd dist"
    echo "  2. Open index.html in browser"
    echo ""
    echo "To distribute:"
    echo "  - Copy all files from dist/ folder"
    echo "  - Or create ZIP: zip -r pfodWeb-standalone.zip dist/*"
    echo ""
    exit 0
else
    echo ""
    echo "========================================"
    echo "  Build Failed!"
    echo "========================================"
    echo ""
    echo "Please check the error messages above"
    echo "and ensure all source files exist."
    echo ""
    exit 1
fi
