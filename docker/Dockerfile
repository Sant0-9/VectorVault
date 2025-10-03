FROM ubuntu:24.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    cmake \
    g++ \
    git \
    ninja-build \
    && rm -rf /var/lib/apt/lists/*

# Create build directory
WORKDIR /build

# Copy source
COPY . .

# Build Release
RUN cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DVECTORVAULT_ENABLE_AVX2=ON \
    -DVECTORVAULT_BUILD_TESTS=OFF \
    -DVECTORVAULT_BUILD_BENCH=OFF \
    -GNinja && \
    cmake --build build -j$(nproc)

# Runtime image
FROM ubuntu:24.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

# Create app user
RUN useradd -m -u 1000 vectorvault

# Copy binary from builder
COPY --from=builder /build/build/vectorvault_api /usr/local/bin/vectorvault_api

# Create data directory
RUN mkdir -p /data && chown vectorvault:vectorvault /data

# Switch to app user
USER vectorvault
WORKDIR /data

# Expose API port
EXPOSE 8080

# Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

# Run server
ENTRYPOINT ["/usr/local/bin/vectorvault_api"]
CMD ["--host", "0.0.0.0", "--port", "8080", "--dim", "384"]
