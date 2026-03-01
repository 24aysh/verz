# `verz write-tree` — Write Working Directory as a Tree Object

## Usage
```bash
verz write-tree
```
Prints the SHA1 of the created tree object.

## What it does
Recursively walks the current working directory, hashes every file as a blob, builds binary tree entries, creates tree objects for every directory level, writes all of them to `.verz/objects/`, and prints the root tree SHA.

## Internal Flow

```
cmd_write_tree()
  └── write_tree(current_path())     ← recursive
        ├── for each entry in directory:
        │     skip ".verz/"
        │     if directory → write_tree(subpath)  (recurse, get subtree sha)
        │     if file:
        │         read content
        │         createBlobObject(content, write=true)  → blob sha + persisted
        │         detect mode (100755 if executable, else 100644)
        │     push TreeEntry{mode, sha_hex, name}
        │
        ├── sort entries alphabetically (TreeEntry::operator<)
        ├── build tree content:
        │     "<mode> <name>\0<20-byte-binary-sha>"  (for each entry)
        └── createTreeObject(tree_content, write=true)  → tree sha + persisted
```

## TreeEntry Struct

```cpp
struct TreeEntry {
  std::string mode;     // "100644", "100755", "040000"
  std::string sha_hex;  // 40-char hex SHA1
  std::string name;     // filename (not full path)
  bool operator<(const TreeEntry &other) const { return name < other.name; }
};
```

## File Modes

| Mode | Meaning |
|---|---|
| `100644` | Regular file (not executable) |
| `100755` | Executable file |
| `040000` | Directory / subtree |

## Helper Functions

| Function | Location | Description |
|---|---|---|
| `createBlobObject(content, write=true)` | `utils.cpp` | Hash + write blob to object store |
| `createTreeObject(entries, write=true)` | `utils.cpp` | Hash + write tree to object store |
| `hexToBinary(sha_hex)` | `utils.cpp` | Converts 40-char hex to 20-byte binary for tree entry encoding |

## Notes
- Both `createBlobObject` and `createTreeObject` are called with `write=true` here, ensuring all referenced objects can be found later by `verz switch` during checkout.
- Skips `.verz/` but does **not** respect a `.verzignore` — all other files are included.
