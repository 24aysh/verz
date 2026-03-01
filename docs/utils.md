# Utility Functions — `utils.h` / `utils.cpp`

Shared utility functions used across all commands.

---

## Conversion Utilities

### `binaryToHex(const std::string &binary) → std::string`
Converts raw binary bytes to a lowercase hex string.

### `hexToBinary(const std::string &hex) → std::string`
Converts a 40-char hex string to a 20-byte binary string. Used when building tree objects (tree entries store 20-byte binary SHAs).

---

## Compression Utilities

### `zlibCompress(const std::string &data) → std::string`
Wraps zlib `deflate()` with `Z_DEFAULT_COMPRESSION`. Returns the compressed bytes.

### `zlibDecompress(const std::string &compressed) → std::string`
Wraps zlib `inflate()`. Reads until `Z_STREAM_END`. Throws `std::runtime_error` on inflate failure.

---

## SHA1 Hashing

### `calcSHA1(const std::string &content) → std::string`
Calls OpenSSL `SHA1()` on the full content bytes, returns a 40-char lowercase hex string.

---

## Git Object I/O

### `getObjectPath(const std::string &hash) → std::string`
Returns `.verz/objects/<hash[0:2]>/<hash[2:]>`.

### `objectExists(const std::string &hash) → bool`
Returns `std::filesystem::exists(getObjectPath(hash))`.

### `readGitObject(const std::string &hash) → std::string`
Reads the compressed object file, decompresses it, and returns the full `"type size\0content"` string (header + null byte + raw content).

### `writeGitObject(const std::string &content, const std::string &hash)`
Creates the `<first2>` subdirectory if needed, then writes `zlibCompress(content)` to the object path.

---

## Higher-Level Object Builders

### `createGitObject(type, content, write=false) → std::string`
1. Builds: `"<type> <content.size()>\0<content>"`
2. `calcSHA1(object)` → 40-char hex hash
3. If `write=true`: calls `writeGitObject(object, hash)`
4. Returns hash

### `createBlobObject(content, write=false) → std::string`
Calls `createGitObject("blob", content, write)`.

### `createTreeObject(entries, write=false) → std::string`
Calls `createGitObject("tree", entries, write)`.  
`entries` is a pre-built binary string of `"<mode> <name>\0<20-byte-sha>"` entries.

---

## Usage Summary

| Called by | Functions used |
|---|---|
| `hash-object` | `createBlobObject`, `calcSHA1` |
| `cat-file` | (internal inflate, not via utils) |
| `ls-tree` | `readGitObject`, `binaryToHex` |
| `write-tree` | `createBlobObject(write=true)`, `createTreeObject(write=true)`, `hexToBinary` |
| `commit-tree` | `calcSHA1`, `writeGitObject` |
| `add` | `createBlobObject(write=true)` |
| `branch/switch` | `readGitObject`, `binaryToHex` |
| `clone` | `binaryToHex` (via length-aware string) |
