# Verz

A lightweight Git implementation in C++ supporting core version control operations — including cloning from GitHub, staging, committing, and branch management.

## Commands

| Command | Description |
|---|---|
| `verz init` | Initialize a new repository |
| `verz register <user> <email>` | Register author identity for commits |
| `verz add <path>...` | Stage files for the next commit |
| `verz commit -m <message>` | Commit staged changes |
| `verz branch` | List all branches |
| `verz branch <name>` | Create a new branch at HEAD |
| `verz switch <name>` | Switch to a branch (restores working tree) |
| `verz delete-branch <name>` | Delete a branch |
| `verz clone <url> <dir>` | Clone a remote GitHub/Git repository |
| `verz cat-file -p <sha>` | Print object contents |
| `verz hash-object [-w] <file>` | Hash a file as a blob object |
| `verz ls-tree [-r\|--name-only] <sha>` | List tree object entries |
| `verz write-tree` | Write working directory as a tree object |
| `verz commit-tree <tree> [-p <parent>] -m <msg>` | Low-level commit creation |

## Requirements

- C++17 compatible compiler (g++, clang++)
- OpenSSL (`libssl`, `libcrypto`) — SHA1 hashing
- zlib (`libz`) — object compression
- libcurl (`libcurl`) — HTTP for `verz clone`

### Installing Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get install build-essential libssl-dev zlib1g-dev libcurl4-openssl-dev
```

**Fedora/RHEL:**
```bash
sudo dnf install gcc-c++ openssl-devel zlib-devel libcurl-devel
```

**Arch Linux:**
```bash
sudo pacman -S base-devel openssl zlib curl
```

**macOS:**
```bash
brew install openssl zlib curl
```

## Building

```bash
make          # default build
make debug    # with debug symbols (-g -O0)
make release  # with optimizations (-O3)
make clean    # remove build artifacts
make rebuild  # clean + build
```

Binary is at `bin/verz`.

## Typical Workflow

```bash
# New repo
verz init
verz register Alice alice@example.com
echo "hello" > README.md
verz add .
verz commit -m "initial commit"

# Branching
verz branch feature
verz switch feature
echo "new feature" > feature.txt
verz add feature.txt
verz commit -m "add feature"
verz switch main

# Cloning
verz clone https://github.com/user/repo ./repo
```

## Repository Structure (`.verz/`)

```
.verz/
├── HEAD              ← "ref: refs/heads/main"
├── index             ← staging area (plain text)
├── objects/          ← content-addressed object store
│   ├── ab/
│   │   └── cd1234…  ← zlib-compressed blob/tree/commit
│   └── …
├── refs/
│   └── heads/
│       ├── main      ← commit sha
│       └── feature   ← commit sha
└── user/
    └── config        ← "username\nemail\n"
```

## Documentation

See [`docs/`](docs/) for detailed internals of each command, including object formats, helper functions, and implementation notes.

- [`docs/internals.md`](docs/internals.md) — deep-dive into all binary/text formats: objects, index, HEAD/refs, packfile, VLI, delta, pkt-line.
- [`docs/verz-vs-git.md`](docs/verz-vs-git.md) — contrast between verz and real Git: what's faithful, what's simplified, and why.

---

For educational purposes — a from-scratch implementation of core Git internals in C++17.
