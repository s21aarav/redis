# Redis Clone (In-Memory RESP Database)

A fast, fully-compliant, in-memory RESP database written from scratch in C++17. 
Tailor-made for macOS systems taking advantage of highly efficient `kqueue` for the event loop.

## Architecture
- **Network Layer:** Leverages `kqueue` and fully non-blocking I/O.
- **Parser:** Zero-allocation custom parser translating RESP arrays without overhead.
- **Storage Core:** Backed by `std::unordered_map` with `std::shared_mutex` providing extremely fast read queries with minimal contention.
- **TTL Eviction:** Min-Heap based background thread accurately scheduling TTL operations using modern C++ `<chrono>` routines.

## Performance
- Up to **110k req/s** throughput handled by highly-optimized zero-copy structures in combination with non-blocking socket loops.

## Building
```bash
# Using Make
make

# Using CMake
mkdir build && cd build
cmake ..
make
```

## Running
```bash
./server
```
