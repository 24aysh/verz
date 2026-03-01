# Verz vs Git — Key Differences

A comparison of how verz implements things versus how real Git does it, and why the simplifications were made.

---

## Index / Staging Area

| Aspect | Git | Verz |
|---|---|---|
| Format | Binary (`DIRCACHE` format) | Plain text (`mode sha path` per line) |
| Stat cache | Stores `ctime`, `mtime`, `inode`, `dev`, `uid`, `gid`, `filesize` per entry | None |
| Dirty detection | Compares stat cache to avoid re-hashing unchanged files | Always re-hashes on `add` |
| Conflict stages | Stores up to 3 versions of a file during merge conflicts (stage 1/2/3) | Not supported |
| Checksum | SHA1 of entire index appended at end | None |
| Flags | `assume-unchanged`, `skip-worktree`, name length, extended flags | None |

**Real Git's binary index entry (per file):**
```
32 bytes  stat data (ctime ns, mtime ns, dev, ino, mode, uid, gid, size)
20 bytes  binary SHA1
 2 bytes  flags
 N bytes  null-terminated path + padding to 8-byte boundary
```

**Verz index line:**
```
100644 a2beefd59223ea16000788d77e62f96bdaf23c7c src/main.cpp
```

**Why:** Stat caching is an optimization. Verz doesn't need it since it re-hashes everything on `add`. Plain text is far simpler to parse and debug.

---

## Object Store

| Aspect | Git | Verz |
|---|---|---|
| Location | `.git/objects/` | `.verz/objects/` |
| Format | Same: `zlib( "type size\0content" )` | Identical |
| Loose objects | Yes | Yes |
| Packfiles | Yes (gc, push, fetch) | Only during `clone` (received, not generated) |
| Delta generation | Yes (for pack) | No — deltas only decoded during `clone` |
| Object types | blob, tree, commit, tag | blob, tree, commit (tag not written) |

**Why:** The core object model is faithfully replicated — this is the heart of Git's design and not something worth simplifying.

---

## Commit

| Aspect | Git | Verz |
|---|---|---|
| Tree source | Staged index only (only what was `git add`-ed) | Full working directory (always `write_tree(cwd)`) |
| GPG signing | Supported | Not supported |
| Multiple parents | Supported (merge commits) | Not supported (single parent only) |
| Encoding header | Can specify text encoding | Always UTF-8 implied |
| Notes | Supported via `refs/notes/` | Not supported |

**Why:** The biggest practical difference — verz commits the entire working tree, not just staged files. This means `verz add` is more of a "permission check" than a true staging operation. Implementing a staged-only tree build would require `write_tree` to read from the index instead of walking the filesystem.

---

## Branching

| Aspect | Git | Verz |
|---|---|---|
| Branch ref storage | `.git/refs/heads/<name>` | `.verz/refs/heads/<name>` — identical |
| Packed refs | `.git/packed-refs` for many branches | Not supported |
| Detached HEAD | Supported (SHA written directly to HEAD) | Not supported |
| Remote-tracking branches | `refs/remotes/origin/main` etc. | Not supported |
| `switch` safety | Refuses to switch if uncommitted changes would be overwritten | No — performs a **hard switch**, discarding working tree changes |
| Reflog | `git reflog` tracks every HEAD movement | No reflog |

**Why:** The hard switch in verz (`remove_all` then checkout) is the most dangerous simplification — real `git switch` carefully merges local changes or aborts. Verz silently overwrites.

---

## Clone

| Aspect | Git | Verz |
|---|---|---|
| Protocol | Smart HTTP, SSH, Git protocol | Smart HTTP only |
| Auth | HTTPS credentials, SSH keys | No authentication (public repos only) |
| Partial clone | `--depth`, `--filter=blob:none` etc. | Full clone only |
| Shallow clones | Yes | No |
| Submodules | Yes | No |
| LFS | Yes (via extension) | No |
| Remote config | Writes `[remote "origin"]` to `.git/config` | Does not write any remote config |
| Negotiation | Multi-round ACK/NAK with have-lines | Single-round: sends `want` + `done` immediately (assumes no common history) |
| Thin packs | Server can send thin delta packs | `thin-pack` capability removed; `ofs-delta` supported |

**Why:** Full protocol negotiation (sending `have` lines for common commits to minimize data transfer) requires a local commit graph to walk. Verz skips this and just asks for everything.

---

## Index File — Binary vs Text (detailed)

Real Git's index is designed for:
1. **Speed at scale** — thousands of files, sub-millisecond status checks by comparing stat data without SHA hashing
2. **Merge support** — multiple staged versions of a conflicted file (stages 1, 2, 3)
3. **Extensions** — cached tree extension (pre-computed tree SHAs per directory to avoid recomputing on commit), resolve-undo, split index for large repos

Verz's text index is sufficient because:
- No dirty-file detection needed (we re-hash on every `add`)
- No merge support
- Small scope (educational implementation)

---

## What Verz Gets Right (faithful to Git)

- ✅ SHA1 content-addressed object model
- ✅ Blob, tree, and commit object formats are **byte-for-byte compatible** with real Git
- ✅ Branch refs as plain text files in `refs/heads/`
- ✅ Symbolic HEAD with `ref: refs/heads/<branch>`
- ✅ Commit format (tree, parent, author, committer, message)
- ✅ Tree binary entry format (`mode name\0 20-byte-sha`)
- ✅ Packfile parsing (PACK magic, VLI headers, OFS/REF delta decoding)
- ✅ Git Smart HTTP protocol (pkt-line, side-band-64k)
- ✅ zlib compression for all objects

## What Verz Doesn't Implement

- ❌ Binary index with stat cache
- ❌ Staging area that's independent of the working tree
- ❌ Merge, rebase, cherry-pick
- ❌ Detached HEAD
- ❌ Remote config / fetch / push
- ❌ Reflog
- ❌ Packfile generation (only consumption)
- ❌ `.gitignore` / `.verzignore` processing
- ❌ Stash
- ❌ Tags (reading supported via packfile, writing not implemented)
- ❌ Authentication
