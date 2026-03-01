# `verz add` — Stage Files for the Next Commit

## Usage
```bash
verz add <file>        # stage a single file
verz add <dir>         # stage all files in a directory recursively
verz add .             # stage everything in the current directory
verz add f1.txt f2.txt # stage multiple files
```

## What it does
Hashes each specified file as a blob object (writing it to `.verz/objects/`), then records an entry in the staging area (`.verz/index`). If a file was previously staged, its entry is updated with the new SHA.

## Internal Flow

```
cmd_add(argc, argv)
  ├── check .verz/ exists
  ├── read_index()                          → load existing staged entries
  ├── for each argv path:
  │     stage_path(target, root, entries)
  │       ├── skip .verz/ and .git/ paths
  │       ├── if directory → recursive_directory_iterator → stage_path each file
  │       └── if file:
  │             read content
  │             createBlobObject(content, write=true)  → sha + persisted blob
  │             detect mode (100755 or 100644)
  │             std::filesystem::relative(path, root)  → repo-relative path
  │             upsert entry in entries vector
  ├── sort entries by path
  └── write_index(entries)                  → overwrite .verz/index
```

## Index File Format (`.verz/index`)

Plain text, one entry per line:
```
<mode> <sha_hex> <repo-relative-path>
```

Example:
```
100644 5761abcfdf0c26a75374c945dfe366eaeee04285 .gitignore
100644 a2beefd59223ea16000788d77e62f96bdaf23c7c README.md
100644 979a1cd784949e8bbee5d848f6abf820e27562b3 src/main.cpp
```

## IndexEntry Struct

```cpp
struct IndexEntry {
  std::string mode;     // "100644" or "100755"
  std::string sha_hex;  // 40-char hex SHA1
  std::string path;     // repo-relative path, e.g. "src/main.cpp"
};
```

## Helper Functions

| Function | Location | Description |
|---|---|---|
| `stage_path(target, root, entries)` | `add.cpp` | Recursively hashes files and upserts `IndexEntry` records |
| `read_index()` | `add.cpp` | Parses `.verz/index` → `vector<IndexEntry>` |
| `write_index(entries)` | `add.cpp` | Truncates and rewrites `.verz/index` |
| `createBlobObject(content, write)` | `utils.cpp` | Hashes content as blob, writes object to disk |
| `should_skip(path)` | `add.cpp` (static) | Returns `true` for `.verz/` and `.git/` paths |

## Notes
- The index is a **flat list** — directory structure is only preserved via the path string.
- `verz add` does NOT consult `.gitignore` style ignore rules.
- Re-staging an already-staged file updates the SHA and mode in-place.
- The index is cleared (`write_index({})`) after each successful `verz commit`.
