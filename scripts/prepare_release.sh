#!/bin/bash
set -e

# VectorVault Release Preparation Script
# Builds release artifacts for Linux and Windows (cross-compile)

VERSION=$(cat VERSION)
RELEASE_DIR="release/v${VERSION}"
BUILD_DIR="build"

echo "================================================"
echo "  VectorVault v${VERSION} Release Preparation"
echo "================================================"
echo ""

# Create release directory
mkdir -p "$RELEASE_DIR"

echo "Step 1: Building Linux x64 Release..."
echo "--------------------------------------"

# Build Release for Linux
cmake -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DVECTORVAULT_ENABLE_AVX2=ON \
    -DVECTORVAULT_BUILD_TESTS=ON \
    -DVECTORVAULT_BUILD_BENCH=ON

cmake --build "$BUILD_DIR" -j$(nproc)

echo ""
echo "Step 2: Running tests..."
echo "------------------------"
cd "$BUILD_DIR"
ctest --output-on-failure
cd ..

echo ""
echo "Step 3: Packaging Linux binaries..."
echo "------------------------------------"

# Copy Linux binaries
cp "$BUILD_DIR/vectorvault_api" "$RELEASE_DIR/vectorvault_api-linux-x64"
cp "$BUILD_DIR/vectorvault_bench" "$RELEASE_DIR/vectorvault_bench-linux-x64"

# Strip binaries
strip "$RELEASE_DIR/vectorvault_api-linux-x64" 2>/dev/null || true
strip "$RELEASE_DIR/vectorvault_bench-linux-x64" 2>/dev/null || true

echo ""
echo "Step 4: Creating tarball..."
echo "---------------------------"

tar -czf "$RELEASE_DIR/vectorvault-${VERSION}-linux-x64.tar.gz" \
    -C "$RELEASE_DIR" \
    vectorvault_api-linux-x64 \
    vectorvault_bench-linux-x64

echo ""
echo "Step 5: Generating checksums..."
echo "--------------------------------"

cd "$RELEASE_DIR"
sha256sum vectorvault-*.tar.gz > checksums.txt
sha256sum vectorvault_*-linux-x64 >> checksums.txt
cd - > /dev/null

echo ""
echo "================================================"
echo "Release artifacts ready in: $RELEASE_DIR"
echo "================================================"
echo ""

ls -lh "$RELEASE_DIR"

echo ""
echo "Next steps:"
echo "  1. Review RELEASE_NOTES.md"
echo "  2. Create git tag: git tag -a v${VERSION} -m 'Release v${VERSION}'"
echo "  3. Push tag: git push origin v${VERSION}"
echo "  4. Create GitHub release and upload artifacts"
echo "  5. Build and push Docker image"
echo ""