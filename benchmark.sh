#!/bin/bash

GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${BLUE}=== File Server Benchmark ===${NC}\n"

echo "Creating test files..."
mkdir -p test_files
echo "<html><body><h1>Test Page</h1></body></html>" > test_files/index.html
dd if=/dev/zero of=test_files/1kb.bin bs=1024 count=1 2>/dev/null
dd if=/dev/zero of=test_files/100kb.bin bs=1024 count=100 2>/dev/null
dd if=/dev/zero of=test_files/1mb.bin bs=1024 count=1024 2>/dev/null
dd if=/dev/zero of=test_files/10mb.bin bs=1024 count=10240 2>/dev/null

DURATION="10s"
CONNECTIONS=100
THREADS=4

echo -e "\nConfiguration:"
echo "  Duration: $DURATION"
echo "  Threads: $THREADS"
echo "  Connections: $CONNECTIONS"
echo ""

if ! command -v wrk &> /dev/null; then
    echo -e "${YELLOW}wrk is not installed. Install with: apt install wrk${NC}"
    exit 1
fi

run_benchmark() {
    local name=$1
    local url=$2
    
    echo -e "${GREEN}Benchmarking $name...${NC}"
    wrk -t$THREADS -c$CONNECTIONS -d$DURATION $url
    echo ""
}

echo -e "${BLUE}=== Test 1: Small HTML file (index.html) ===${NC}"
run_benchmark "C Server" "http://localhost:8000/test_files/index.html"
run_benchmark "Python Server" "http://localhost:8001/test_files/index.html"

echo -e "${BLUE}=== Test 2: 1KB binary file ===${NC}"
run_benchmark "C Server" "http://localhost:8000/test_files/1kb.bin"
run_benchmark "Python Server" "http://localhost:8001/test_files/1kb.bin"

echo -e "${BLUE}=== Test 3: 100KB file ===${NC}"
run_benchmark "C Server" "http://localhost:8000/test_files/100kb.bin"
run_benchmark "Python Server" "http://localhost:8001/test_files/100kb.bin"

echo -e "${BLUE}=== Test 4: 1MB file ===${NC}"
run_benchmark "C Server" "http://localhost:8000/test_files/1mb.bin"
run_benchmark "Python Server" "http://localhost:8001/test_files/1mb.bin"

echo -e "${BLUE}=== Test 5: 10MB file (IO bottleneck test) ===${NC}"
run_benchmark "C Server" "http://localhost:8000/test_files/10mb.bin"
run_benchmark "Python Server" "http://localhost:8001/test_files/10mb.bin"

echo -e "${BLUE}=== Test 6: Directory listing ===${NC}"
run_benchmark "C Server" "http://localhost:8000/test_files/"
run_benchmark "Python Server" "http://localhost:8001/test_files/"

echo -e "${BLUE}=== Benchmark Complete ===${NC}"
echo "Cleaning up test files"
rm -rf test_files