# `verz branch`, `verz switch`, `verz delete-branch` — Branch Management

## Commands

```bash
verz branch                    # list all branches (* marks current)
verz branch <name>             # create a new branch at current HEAD
verz switch <name>             # switch to a branch + restore working tree
verz delete-branch <name>      # delete a branch ref
```

---

## `verz branch`

### Create a branch
```
cmd_branch(argc, argv)
  ├── validate name (no '/' or spaces)
  ├── check branch doesn't already exist
  ├── get_head_commit()          → current HEAD sha (must be non-empty)
  ├── create .verz/refs/heads/<name>
  └── write sha to ref file
```

### List branches
```
cmd_branch(argc=2, argv)
  ├── current_branch()           → parse .verz/HEAD for active branch
  ├── iterate .verz/refs/heads/
  └── print "* <name>" or "  <name>" for each
```

---

## `verz switch`

```
cmd_switch(argc, argv)
  ├── check branch ref exists
  ├── check not already on that branch
  ├── overwrite .verz/HEAD: "ref: refs/heads/<name>"
  ├── read_branch_sha(refPath)   → new branch's commit sha
  └── checkout_commit(sha, cwd)
        ├── readGitObject(commitSha)     → strip header → get "tree <sha>"
        ├── for all entries in cwd except .verz → remove_all
        └── checkout_tree(treeSha, root)
              ├── readGitObject(treeSha) → strip header → binary tree data
              ├── parse entries: <mode> SP <name> NUL <20-byte-sha>
              └── for each entry:
                    binaryToHex(binSha)
                    if mode == 40000 → create_directories + recurse
                    else → readGitObject(blobSha) → strip header → write file
```

### Working Tree Checkout Detail

`checkout_tree()` reads each tree entry's 20-byte binary SHA, converts it to hex, and uses `readGitObject()` to fetch the decompressed content. For blobs, it strips the `"blob <size>\0"` header and writes the raw bytes to the filesystem.

---

## `verz delete-branch`

```
cmd_delete_branch(argc, argv)
  ├── check not deleting the current branch
  ├── check branch ref file exists
  ├── read sha (for display in output)
  └── filesystem::remove(refPath)
```

---

## Helper Functions

| Function | Location | Description |
|---|---|---|
| `current_branch()` | `branch.cpp` | Reads `.verz/HEAD`, extracts `refs/heads/<name>` |
| `branch_ref_path(name)` | `branch.cpp` | Returns `.verz/refs/heads/<name>` |
| `read_branch_sha(refPath)` | `branch.cpp` (static) | Reads sha from a branch ref file |
| `checkout_commit(sha, root)` | `branch.cpp` (static) | Restores working tree from a commit sha |
| `checkout_tree(treeSha, dir)` | `branch.cpp` (static) | Recursively writes tree entries to filesystem |
| `get_head_commit()` | `commit.cpp` | Gets current branch's commit sha |
| `readGitObject(sha)` | `utils.cpp` | Reads + decompresses object, returns `"type size\0content"` |
| `binaryToHex(str, len)` | `utils.cpp` | Converts 20-byte binary SHA to 40-char hex (must use `std::string(ptr, 20)` form to avoid null truncation) |

## Branch Ref Storage

```
.verz/
└── refs/
    └── heads/
        ├── main     ← "abc1234...40chars\n"
        └── feature  ← "def5678...40chars\n"
```

Each file contains the 40-char hex SHA of the tip commit of that branch.

## Notes
- `verz switch` performs a **hard checkout** — it deletes everything in the working tree (except `.verz/`) and restores from the target branch's commit tree. Unstaged changes will be lost.
- Creating a branch (`verz branch <name>`) only creates the ref — it does not switch to it.
- A branch can only be deleted if you are not currently on it.
