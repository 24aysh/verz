# `verz register` — Register User Credentials

## Usage
```bash
verz register <username> <email>
```

## What it does
Stores the user's name and email in `.verz/user/config` so that commits can be attributed to an author.

**Config file format** (`.verz/user/config`):
```
<username>
<email>
```

## Internal Flow

```
cmd_register(argc, argv)
  └── registerUserName(argv[2])   → creates .verz/user/ dir, writes username + \n
  └── registerUserEmail(argv[3])  → appends email + \n
```

### Helper Functions (`src/cmd/register.cpp`)

| Function | Description |
|---|---|
| `registerUserName(username)` | Creates `.verz/user/` directory, writes username as the first line of `config` |
| `registerUserEmail(email)` | Appends email as the second line of `config` (opens in append mode) |

## Notes
- Calling `register` again **overwrites** the username (truncates and rewrites the file) but then appends the email, resulting in correct two-line output.
- `commit.cpp` reads this config via `getUserName()` and `getUserEmail()` from `commit_tree.cpp`.

## Dependencies
- Used by: `verz commit`, `verz commit-tree`
- Reads from: `commit_tree.cpp` → `getUserConfig()`, `getUserName()`, `getUserEmail()`
