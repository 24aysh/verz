# Verz

A lightweight Git implementation in C++ that provides core version control functionality.

## Features

- **init** - Initialize a new Git repository
- **hash-object** - Compute object ID and optionally create a blob from a file
- **cat-file** - Display contents of repository objects

## Requirements

- C++17 compatible compiler (g++, clang++)
- OpenSSL library (for SHA1 hashing)
- zlib library (for compression)

### Installing Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get install build-essential libssl-dev zlib1g-dev
```

**Fedora/RHEL:**
```bash
sudo dnf install gcc-c++ openssl-devel zlib-devel
```

**macOS:**
```bash
brew install openssl zlib
```

## Building

```bash
# Build the project
make

# Build with debug symbols
make debug

# Build with optimizations
make release

# Clean build artifacts
make clean

# Rebuild from scratch
make rebuild
```

The compiled binary will be located at `bin/verz`.

## Usage

### Initialize a Repository

```bash
./bin/verz init
```

Creates a `.verz` directory with the basic repository structure:
- `.verz/objects/` - Object database
- `.verz/refs/` - References
- `.verz/HEAD` - Current branch reference

## Project Structure

```
verz/
├── bin/                    # Build output directory
│   └── obj/               # Object files
├── include/               # Header files
│   ├── init.h
│   ├── cat_file.h
│   └── hash_object.h
├── src/                   # Source files
│   ├── main.cpp
│   └── cmd/              # Command implementations
│       ├── init.cpp
│       ├── cat_file.cpp
│       └── hash_object.cpp
├── Makefile              # Build configuration
└── README.md
```

## Installation

To install verz system-wide (requires sudo):

```bash
make install
```

This installs the binary to `/usr/local/bin/verz`.

To uninstall:

```bash
make uninstall
```

This project is for educational purposes.
