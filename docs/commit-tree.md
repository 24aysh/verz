# `verz commit-tree` — Create a Commit Object Directly

## Usage
```bash
verz commit-tree <tree-sha> -m <message>
verz commit-tree <tree-sha> -p <parent-sha> -m <message>
```

## What it does
Low-level command that directly creates a commit object from a given tree SHA, optional parent SHA, and message. Used internally by `verz commit`. Does **not** update HEAD or clear the index.

## Internal Flow

```
cmd_commit_tree(argc, argv)
  ├── parse flags: -p <parent> and -m <message>
  └── commit_tree(treeHash, parentHash, message)
        ├── reads user config: getUserName(), getUserEmail()
        ├── getTimestamp()  → Unix epoch
        ├── getTimezone()   → "+0530" style
        ├── builds commit content string:
        │     "tree <hash>\n"
        │     ["parent <hash>\n"]
        │     "author <name> <email> <ts> <tz>\n"
        │     "committer <name> <email> <ts> <tz>\n"
        │     "\n"
        │     "<message>\n"
        ├── builds full object: "commit <size>\0<content>"
        ├── calcSHA1(object)
        └── writeGitObject(object, hash)
```

## Commit Object Format

```
tree <40-char-tree-sha>
parent <40-char-parent-sha>     ← omitted if first commit
author Name <email> <timestamp> <timezone>
committer Name <email> <timestamp> <timezone>

<commit message>
```

Stored as zlib-compressed at `.verz/objects/<sha[0:2]>/<sha[2:]>`.

## Helper Functions

| Function | Location | Description |
|---|---|---|
| `commit_tree(tree, parent, msg)` | `commit_tree.cpp` | Core commit object builder |
| `getUserName()` | `commit_tree.cpp` | Reads line 1 of `.verz/user/config` |
| `getUserEmail()` | `commit_tree.cpp` | Reads line 2 of `.verz/user/config`, wraps in `<>` |
| `getUserConfig()` | `commit_tree.cpp` | Reads full `.verz/user/config` as string |
| `getTimestamp()` | `commit_tree.cpp` | `std::time(nullptr)` → Unix epoch string |
| `getTimezone()` | `commit_tree.cpp` | `std::strftime(..., "%z", ...)` → `+HHMM` |
| `calcSHA1(data)` | `utils.cpp` | OpenSSL SHA1 |
| `writeGitObject(content, hash)` | `utils.cpp` | zlib-compress + write to `.verz/objects/` |
