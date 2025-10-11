[![Docker Image Size](https://img.shields.io/docker/image-size/alsca183/yaths)](https://hub.docker.com/r/alsca183/yaths) [![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
# YaTHS - Yet another Tiny HTTP-Server

Ultra-lightweight HTTP file server for directory browsing and local development

Size: 37kb (Musl+strip+upx)
Speed: 12x faster than Python's http.server 
Usecase: Testing, development, quick file sharing, embedded, minimal docker
Features: 
- Directory browsing with minimal/modern CSS styling
- Mobile friendly
- Common MIME types handling
- Directory traversal protection
- Hidden file option
- Fork-based concurrency
- Fully self contained (statically linked)
- Memory footprint: 240 kB

## Quickstart (Docker pre-built)
Docker image size: 35.7KiB
Memory usage: 496KiB
```bash
docker run -p 0.0.0.0:8000:8000 -v /path/to/files/:/data alsca183/yaths
```

## Screenshot
![YaTHS](./yaths.png?raw=true "YaTHS")

## Build from Source

### Prerequisites
- musl-gcc (musl-tools package)
- upx (optional, for compression)

### Linux
```bash
sudo apt install musl-tools upx  # Debian/Ubuntu
# or
sudo dnf install musl-gcc upx    # Fedora

git clone https://github.com/yourusername/yaths
cd yaths
musl-gcc -static -Os -s -o yaths yaths.c
strip yaths
upx --best --lzma yaths
./yaths
```

### macOS
```bash
brew install musl-cross upx
musl-gcc -static -Os -s -o yaths yaths.c
strip yaths
upx --best --lzma yaths
./yaths
```

### No musl-tools
```bash
# Build with gcc
gcc -static -Os yaths.c -o yaths
./yaths
```

### Termux
Be sure that you ran "termux-setup-storage" before serving files
```bash
# Run it in /home
gcc yaths.c -o yaths
./yaths --dir ~/storage
```

## Usage

```
./yaths -h
Usage: ./yaths [OPTIONS] [PORT]

YaTHS - Yet another Tiny HTTP-Server

Options:
  --port PORT        Set port number (default: 8000)
  --dir PATH         Set directory to serve (default: current dir)
  --show-hidden, -a  Show hidden files (files starting with .)
  --help, -h         Show this help message

Examples:
  ./yaths                      # Serve current directory on port 8000
  ./yaths 3000                 # Serve on port 3000
  ./yaths --port 8080          # Serve on port 8080
  ./yaths --dir /var/www       # Serve /var/www directory
  ./yaths -a --port 3000       # Show hidden files on port 3000
```

## Benchmark
YaTHS vs python http-server

| Test | Server | Requests/s | Avg Latency | Transfer/sec | Speedup |
|------|--------|----:|------------:|-------------:|--------:|
| **Small HTML** | YaTHS | 37,416 | 14.8 ms | 4.57 MB | **12.3x** |
| | http-server | 3,050 | 15.7 ms | 685 KB | 1x |
| **1 KB binary** | YaTHS | 36,453 | 16.2 ms | 38.59 MB | **12.1x** |
| | http-server | 3,005 | 22.6 ms | 3.51 MB | 1x |
| **100 KB** | YaTHS | 33,162 | 26.3 ms | 3.17 GB | **11.9x** |
| | http-server | 2,782 | 14.8 ms | 272 MB | 1x |
| **1 MB** | YaTHS | 21,206 | 2.1 ms | 20.71 GB | **11.8x** |
| | http-server | 1,791 | 36.6 ms | 1.75 GB | 1x |
| **10 MB** | YaTHS | 2,912 | 15.8 ms | 28.45 GB | **6.6x** |
| | http-server | 439 | 115.2 ms | 4.32 GB | 1x |
| **Directory listing** | YaTHS | 37,411 | 17.7 ms | 4.57 MB | **13.2x** |
| | http-server | 2,838 | 21.9 ms | 638 KB | 1x |

*Benchmarked with wrk (10s, 4 threads, 100 connections)

## Runtime example:

- VmSize: 240kB - Total virtual memory
- VmRSS: 0kB - Physical memory
- VmData: 8kB - Data segment
- VmStk: 136kB - Stack space allocated
- VmExe: 40kB - Executable code
- VmLib: 24kB - Musl libc

## Docker

```
# Build
docker build -t yaths .
# Run
docker run -p 0.0.0.0:8000:8000 -v /path/to/files/:/data yaths
```

## Security / Limitations

**Development Use Only** - YaTHS is designed for local development and testing. Not recommended for production use:
- No HTTPS/TLS support
- No HTTP/2 or HTTP/3
- No authentication
- No rate limiting
- No compression
- No caching headers
- Minimal hardening

## License
MIT License