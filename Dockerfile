# Dockerfile for high-performance HTTP server
# Uses Alpine Linux for a lightweight image

FROM alpine:latest AS builder

# Install build dependencies
RUN apk add --no-cache \
    build-base \
    cmake \
    g++ \
    make \
    linux-headers

# Set working directory
WORKDIR /app

# Copy source files
COPY . .

# Create build directory and compile
RUN mkdir -p build && \
    cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . -j$(nproc)

# Final image
FROM alpine:latest

# Install only necessary runtime dependencies
RUN apk add --no-cache \
    libstdc++ \
    libgcc \
    && rm -rf /var/cache/apk/*

# Create non-root user for security
RUN addgroup -S appgroup && \
    adduser -S appuser -G appgroup

WORKDIR /app

# Copy executable from builder
COPY --from=builder /app/build/HighPerformanceHttpServer /app/http-server

# Change permissions
RUN chown appuser:appgroup /app/http-server && \
    chmod +x /app/http-server

USER appuser

# Expose port 8080
EXPOSE 8080

# Default command (port 8080, thread pool = CPU core count)
CMD ["./http-server"]
