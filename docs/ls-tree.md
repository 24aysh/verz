# `verz ls-tree` — List Tree Object Contents

## Usage
```bash
verz ls-tree <tree-sha>              # list top-level entries
verz ls-tree --name-only <tree-sha>  # names only, no metadata
verz ls-tree -r <tree-sha>           # recursive (walks into subtrees)
```

## What it does
Reads a tree object from the object store and prints its entries — mode, type, SHA, and filename — one per line. With `-r`, recurses into subtrees.

## Internal Flow

```
cmd_ls_tree(argc, argv)
  └── ls_tree(hash, flag, prefix="")
        ├── readGitObject(hash)  → decompressed string
        ├── verifies header starts with "tree "
        ├── parses binary tree entries:
        │     <mode> SP <name> NUL <20-byte-binary-sha>  (repeated)
        ├── for each entry:
        │     binaryToHex(binSha) → 40-char hex
        │     type = "tree" if mode==40000, else "blob"
        └── prints / recurses based on flags
```

## Tree Object Binary Format

After `"tree <size>\0"` header, entries are packed as:
```
<mode_string> <SP> <name_string> <NUL> <20_byte_binary_sha1>
```

e.g. `40000 src\0<20 bytes>` for a directory, `100644 main.cpp\0<20 bytes>` for a file.

## Internal Struct

```cpp
struct treeObject {
  std::string mode;
  std::string type;    // "blob" or "tree"
  std::string hash;    // 40-char hex
  std::string fileName;
};
```

## Helper Functions

| Function | Location | Description |
|---|---|---|
| `readGitObject(hash)` | `utils.cpp` | Reads + zlib-decompresses object file, returns `"type size\0content"` |
| `binaryToHex(str)` | `utils.cpp` | Converts raw 20-byte binary SHA to 40-char lowercase hex |
| `ls_tree(hash, flag, prefix)` | `ls_tree.cpp` | Recursive tree walker used by `verz switch` checkout too |
