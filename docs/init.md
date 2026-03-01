# `verz init` — Initialize a Repository

## Usage
```bash
verz init
```

## What it does
Creates the `.verz/` directory structure that forms the repository's internal database:

```
.verz/
├── objects/    ← object store (blobs, trees, commits)
├── refs/       ← branch references
└── HEAD        ← pointer to the current branch
```

`HEAD` is initialized to `ref: refs/heads/main`, pointing to the `main` branch (which has no ref file yet until the first commit).

## Source
`src/cmd/init.cpp` → `cmd_init()`

```cpp
std::filesystem::create_directory(".verz");
std::filesystem::create_directory(".verz/objects");
std::filesystem::create_directory(".verz/refs");

std::ofstream headFile(".verz/HEAD");
headFile << "ref: refs/heads/main\n";
```

## Notes
- Safe to call in a directory that already has a `.verz/` — it will not overwrite `HEAD` or objects.
- Does **not** create `.verz/refs/heads/` — that is created on first commit by `update_head()` in `commit.cpp`.
