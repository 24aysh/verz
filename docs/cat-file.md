# `verz cat-file` — Display Object Contents

## Usage
```bash
verz cat-file -p <sha>    # pretty-print object content
```

## What it does
Reads, decompresses, and prints the raw content of any object in the `.verz/objects/` store — blobs, trees, or commits.

## Internal Flow

```
cmd_cat_file(argc, argv)
  └── getGitBlob(".verz/objects/<sha[0:2]>/<sha[2:]>")
        ├── reads compressed file
        ├── zlib inflate (decompresses)
        ├── finds null byte → strips "type size\0" header
        └── returns raw content string
```

## Object Layout on Disk

All objects are stored as:
```
zlib_compress( "<type> <size>\0<raw_content>" )
```

Path: `.verz/objects/<first2>/<last38>`

## Static Helper

| Function | Description |
|---|---|
| `getGitBlob(filepath)` | Opens file, zlib-inflates it, strips the header before `\0`, returns content |

This is an **internal static function** in `cat_file.cpp` (not reused elsewhere — note: `readGitObject` in `utils.cpp` does the same thing and is used by `ls_tree`, `branch`, etc.).

## Example Output

For a **commit** object:
```
tree 41f4c7755e270a7ebf14f7d8860dddfb7e570abc
author Alice <alice@example.com> 1700000000 +0530
committer Alice <alice@example.com> 1700000000 +0530

initial commit
```

For a **blob**:
```
int main() { return 0; }
```
