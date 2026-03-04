# Build Instructions

## Prerequisites
- CMake 3.10+
- Ninja build system
- C++ compiler (C++17 or later)

## Build Steps

```bash
mkdir build
cd build
cmake -G Ninja .. 
ninja
```

or
```bash
mkdir build
cd build
cmake -G Ninja -DDEBUG=1 ..
ninja
```

## Installation

```bash
ninja install
```