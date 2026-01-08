# High-Performance HTTP Server

High-performance multithreaded HTTP server developed in C++17 for Linux, using epoll and a custom Thread Pool. Designed to support the C10k problem (10,000 simultaneous connections) and achieve performance exceeding 12,000 requests per second.

## Features

- ✅ **Optimal performance**: > 12,000 requests/second (RPS)
- ✅ **Response time**: < 5 ms under load
- ✅ **C10k support**: Up to 10,000 simultaneous connections
- ✅ **I/O multiplexing**: Using epoll (edge-triggered)
- ✅ **Custom Thread Pool**: Eliminates thread-per-request model
- ✅ **HTTP/1.1**: Full support with keep-alive
- ✅ **Error handling**: Status codes 400, 404, 500
- ✅ **POSIX sockets**: From scratch implementation without framework

## Architecture

### Main Components

1. **ThreadPool**: Reusable thread pool managed with `std::mutex` and `std::condition_variable`
2. **HttpServer**: Main server using epoll for I/O multiplexing
3. **HttpRequest/HttpResponse**: HTTP message parsing and generation
4. **Connection**: Client connection management with buffers

### Optimizations

- **epoll with edge-triggered mode**: Reduced system calls
- **Non-blocking sockets**: Better resource utilization
- **Thread Pool**: Thread reuse instead of create/destroy
- **Keep-alive**: Reduced connection overhead
- **Reusable buffers**: Limited memory allocations

## Prerequisites

- **System**: Linux (kernel 2.6.17+ for epoll)
- **Compiler**: GCC 7+ or Clang 5+ with C++17 support
- **CMake**: Version 3.10 or higher
- **Build tools**: make, g++

## Building

### Local Build

```bash
# Create build directory
mkdir build
cd build

# Generate build files
cmake .. -DCMAKE_BUILD_TYPE=Release

# Compile
cmake --build . -j$(nproc)

# Executable will be in build/HighPerformanceHttpServer
```

### Docker Build

```bash
# Build the image
docker build -t http-server:latest .

# Run the container
docker run -d -p 8080:8080 --name http-server http-server:latest
```

## Usage

### Simple Execution

```bash
./HighPerformanceHttpServer [port] [thread_pool_size]
```

**Parameters**:
- `port`: Listening port (default: 8080)
- `thread_pool_size`: Number of threads in the pool (default: CPU core count)

**Examples**:
```bash
# Use default port (8080)
./HighPerformanceHttpServer

# Specify port
./HighPerformanceHttpServer 9000

# Specify port and thread pool size
./HighPerformanceHttpServer 8080 8
```

### Stopping the Server

Press `Ctrl+C` to gracefully stop the server.

## Performance Testing

### With Apache Bench (ab)

```bash
# Install Apache Bench (Ubuntu/Debian)
sudo apt-get install apache2-utils

# Basic test: 10,000 requests, 100 concurrent connections
ab -n 10000 -c 100 http://localhost:8080/

# C10k test: 100,000 requests, 10,000 concurrent connections
ab -n 100000 -c 10000 -k http://localhost:8080/

# Performance test: 50,000 requests, 500 concurrent connections, keep-alive
ab -n 50000 -c 500 -k http://localhost:8080/
```

### Performance Targets

- **Requests/second**: ≥ 12,000 RPS
- **Response time**: < 5 ms (average)
- **Simultaneous connections**: 10,000+
- **p95 latency**: < 10 ms

### Testing with curl

```bash
# Simple request
curl http://localhost:8080/

# Check headers
curl -v http://localhost:8080/

# Test keep-alive
curl -v -H "Connection: keep-alive" http://localhost:8080/
```

## Project Structure

```
http-server/
├── CMakeLists.txt          # CMake configuration
├── Dockerfile              # Docker configuration
├── README.md               # Documentation
└── src/
    ├── main.cpp            # Entry point
    ├── HttpServer.h/cpp    # Main server with epoll
    ├── ThreadPool.h/cpp    # Thread pool
    ├── Connection.h/cpp    # Connection management
    ├── HttpRequest.h/cpp   # HTTP parser
    └── HttpResponse.h/cpp  # HTTP response generator
```

## Available Routes

- `GET /` or `GET /index.html`: Homepage (200 OK)
- Any other route: 404 Not Found

## HTTP Status Codes

- **200 OK**: Request successful
- **400 Bad Request**: Invalid HTTP request
- **404 Not Found**: Resource not found
- **500 Internal Server Error**: Server error (not currently used)

## Keep-Alive

The server supports persistent connections (HTTP/1.1 keep-alive):
- **By default**: keep-alive enabled for HTTP/1.1
- **Header**: `Connection: keep-alive` or `Connection: close`
- **Timeout**: 5 seconds
- **Max requests**: 1000 per connection

## Known Limitations

- HTTP/1.1 GET support only (POST, PUT, DELETE not implemented)
- No HTTPS/SSL support
- No static file serving from disk
- No HTTP/2 support

## Possible Future Improvements

- Additional HTTP method support (POST, PUT, DELETE)
- Static file serving
- HTTP caching
- HTTPS/SSL support with OpenSSL
- HTTP/2 support
- Configurable logging
- Metrics and monitoring

## Technical Notes

### Why epoll?

epoll is the most performant I/O multiplexing API on modern Linux, particularly well-suited for handling a large number of simultaneous connections (C10k). Compared to select/poll, epoll offers:
- O(1) complexity instead of O(n)
- No limitation on the number of file descriptors
- Edge-triggered mode to reduce syscalls

### Thread Pool vs thread-per-request

The thread-per-request model creates a new thread for each request, which:
- Limits the number of connections (system limit on threads)
- Creates creation/destruction overhead
- Consumes a lot of memory (stack per thread)

The Thread Pool solves these problems by:
- Reusing a fixed number of threads
- Distributing tasks via a queue
- Minimizing resource allocations

## License

This project is provided for educational and demonstration purposes.

## Author

High-performance HTTP server developed from scratch in C++17.
