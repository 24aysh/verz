# Verz

A lightweight Git implementation in C++ that provides core version control functionality.

## Features

- **init** - Initialize a new repository
- **register** - Register user credentials (username and email)
- **hash-object** - Compute object ID and optionally create a blob from a file
- **cat-file** - Display contents of repository objects
- **ls-tree** - List the contents of a tree object
- **write-tree** - Create a tree object from the current index
- **commit-tree** - Create a new commit object

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
│   ├── cat_file.h
│   ├── commit_tree.h
│   ├── hash_object.h
│   ├── init.h
│   ├── ls_tree.h
│   ├── register.h
│   ├── utils.h
│   └── write_tree.h
├── src/                   # Source files
│   ├── main.cpp
│   └── cmd/              # Command implementations
│       ├── cat_file.cpp
│       ├── commit_tree.cpp
│       ├── hash_object.cpp
│       ├── init.cpp
│       ├── ls_tree.cpp
│       ├── register.cpp
│       └── write_tree.cpp
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
