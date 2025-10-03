#!/bin/bash
set -e

# VectorVault Docker Release Script

VERSION=$(cat VERSION)
IMAGE_NAME="vectorvault"
REGISTRY="ghcr.io/sant0-9"  # Update with your registry

echo "================================================"
echo "  VectorVault v${VERSION} Docker Build"
echo "================================================"
echo ""

echo "Building Docker image..."
docker build -t "${IMAGE_NAME}:${VERSION}" -f docker/Dockerfile .
docker tag "${IMAGE_NAME}:${VERSION}" "${IMAGE_NAME}:latest"

echo ""
echo "Testing Docker image..."
docker run --rm -d --name vectorvault-test -p 8080:8080 "${IMAGE_NAME}:${VERSION}" --dim 384

# Wait for container to start
sleep 3

# Test health endpoint
if curl -f http://localhost:8080/health > /dev/null 2>&1; then
    echo "✓ Health check passed"
else
    echo "✗ Health check failed"
    docker stop vectorvault-test
    exit 1
fi

# Stop test container
docker stop vectorvault-test

echo ""
echo "================================================"
echo "Docker image ready: ${IMAGE_NAME}:${VERSION}"
echo "================================================"
echo ""

echo "Usage:"
echo "  docker run -p 8080:8080 -v \$(pwd)/data:/data ${IMAGE_NAME}:${VERSION} --dim 384"
echo ""

echo "To push to registry:"
echo "  docker tag ${IMAGE_NAME}:${VERSION} ${REGISTRY}/${IMAGE_NAME}:${VERSION}"
echo "  docker push ${REGISTRY}/${IMAGE_NAME}:${VERSION}"
echo "  docker push ${REGISTRY}/${IMAGE_NAME}:latest"
echo ""