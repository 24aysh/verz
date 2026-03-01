# `verz commit` — Record Staged Changes as a Commit

## Usage
```bash
verz commit -m "<message>"
```

## What it does
Reads the staged index, builds a tree object from the current working directory, creates a commit object chaining from HEAD, updates the branch ref, and clears the index.

## Internal Flow

```
cmd_commit(argc, argv)
  ├── check .verz/ exists
  ├── check .verz/user/config exists   (user must be registered)
  ├── parse -m <message>
  ├── read_index()                     → if empty: "nothing to commit", exit 0
  ├── write_tree(current_path())       → tree sha (all blobs+trees persisted)
  ├── get_head_commit()                → parent sha (empty string if first commit)
  ├── commit_tree(treeHash, parentHash, message)   → commit sha + persist
  ├── update_head(commitHash)          → write sha to branch ref file
  ├── write_index({})                  → clear the staging area
  └── print "[<branch> <short-sha>] <message>"
```

## HEAD Resolution

HEAD is resolved in a chain:
```
.verz/HEAD          → "ref: refs/heads/main"
.verz/refs/heads/main → "<40-char commit sha>"
```

### `resolve_head_ref_path()` (static in `commit.cpp`)
Reads `.verz/HEAD`, strips `"ref: "` prefix, returns `.verz/<refpath>`.

### `get_head_commit()`
Opens the resolved ref file; returns `""` if the file doesn't exist (fresh repo = no parent).

### `update_head(sha)`
Creates `.verz/refs/heads/` if it doesn't exist, then writes the new SHA to the ref file.

## Helper Functions

| Function | Location | Description |
|---|---|---|
| `resolve_head_ref_path()` | `commit.cpp` (static) | Resolves symbolic HEAD to a ref file path |
| `get_head_commit()` | `commit.cpp` | Returns current HEAD commit sha (empty if none) |
| `update_head(sha)` | `commit.cpp` | Writes sha to branch ref, creating dirs if needed |
| `read_index()` | `add.cpp` | Loads `.verz/index` entries |
| `write_index({})` | `add.cpp` | Clears `.verz/index` after commit |
| `write_tree(path)` | `write_tree.cpp` | Builds tree objects from working directory |
| `commit_tree(tree, parent, msg)` | `commit_tree.cpp` | Creates commit object content + SHA |

## Notes
- `write_tree` is called on the full working tree (not just staged files). This means all current files are included, matching the staged snapshot.
- After a commit, the index is empty — run `verz add` again before the next commit.
- Does not support staged-only trees (unlike real git); the full FS tree is always committed.
