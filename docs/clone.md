# `verz clone` — Clone a Remote Git Repository

## Usage
```bash
verz clone <url> <directory>
```

## What it does
Clones a public GitHub repository (or any Git HTTP server) into a local directory by implementing the Git Smart HTTP protocol from scratch.

## Internal Flow Overview

```
clone(url, root)
  ├── curl_global_init / curl_easy_init
  ├── discoverRefs(curl, url)           → GET /info/refs?service=git-upload-pack
  ├── readRefs(responseBuffer)          → map<refName, oid>
  ├── resolveHead(refs)                 → find HEAD branch
  ├── build upload-pack request:
  │     "want <oid> side-band-64k ofs-delta\n"  + "0000" + "done\n"
  ├── makeRequest(curl, request, url)   → POST /git-upload-pack → packfile bytes
  ├── initialiseGitRepo(root, refs)     → create .git dirs, write HEAD, return commitSha
  └── parsePackFile(parser, commitSha, root)
```

---

## Phase 1 — Ref Discovery (`discoverRefs`)

Sends a `GET` to `<url>/.git/info/refs?service=git-upload-pack`.

**Response format (pkt-line):**
```
<4-hex-len><data>     ← repeated
0000                  ← flush packet
```

`readPktLine()` parses each pkt-line by reading the 4-byte hex length prefix, then the payload.

`readRefs()` builds a `map<refName, oid>` from the response. The first real line contains capabilities after a NUL byte (stripped). Subsequent lines are `<oid> <refname>\n`.

`resolveHead()` finds the ref that corresponds to HEAD by detecting the trailing space sentinel added during parsing.

---

## Phase 2 — Pack Negotiation (`makeRequest`)

Sends a `POST` to `<url>/.git/git-upload-pack` with body:
```
<pkt-line: "want <oid> side-band-64k ofs-delta\n">
0000
<pkt-line: "done\n">
```

The response is streamed via `writeCallbackPackFileReceive` → `parseRecToPktLine` → `handlePktLine`, which:
- In `READ_ACK` phase: waits for `NAK`, then transitions to `READ_SIDE_BAND`
- In `READ_SIDE_BAND` phase: reads multiplexed band data:
  - Band `\x01` → packfile bytes appended to `parser.packfile`
  - Band `\x02` → progress messages (ignored)
  - Band `\x03` → remote errors (thrown as exception)

---

## Phase 3 — Packfile Parsing (`parsePackFile`)

Validates `PACK` magic, reads version and object count. For each object:

### Object Header (VLI encoding)
```
bits [6:4] = type   (1=commit, 2=tree, 3=blob, 4=tag, 6=ofs-delta, 7=ref-delta)
bits [3:0] = size[3:0]
if MSB set: next byte contributes size[10:4], etc. (7 bits each)
```

### Object Types

| Type | Handling |
|---|---|
| 1–4 (base objects) | `decompress_continuous()` → hash with SHA1 → `createGitObject()` |
| 6 (OFS-delta) | Read negative offset → find base in `objectCache` → `resolveDelta()` |
| 7 (REF-delta) | Read 20-byte base SHA → find base in `hashCache` → `resolveDelta()` |

### Delta Resolution (`resolveDelta`)

Interprets the delta stream against a base object:
- Reads source size and target size (VLI encoded)
- For each instruction:
  - If MSB set → **copy** instruction: reads up to 4-byte offset + 3-byte size from base
    - `copySize == 0` treated as `0x10000` (git spec)
  - If MSB clear → **insert** instruction: `opcode & 0x7F` bytes directly from delta stream

### Caches

| Cache | Key | Value |
|---|---|---|
| `objectCache` | `size_t` (packfile byte offset) | Decompressed object bytes |
| `hashCache` | `std::string` (40-char hex sha) | Packfile byte offset |
| `typeCache` | `size_t` (packfile byte offset) | Object type (1–4) |

### SHA1 Hashing Note
All SHA1 hashes on 20-byte arrays use `std::string(ptr, 20)` (not `const char*`) to avoid truncation at embedded null bytes.

---

## Phase 4 — Tree Checkout (`parseTree`)

After all objects are parsed:
```
commitData = objectCache[hashCache[commitSha]]  → raw commit bytes
extract tree sha from "tree <sha>\n..."
parseTree(treeSha, root+"/", hashCache, objectCache)
  ├── read binary tree entries
  ├── for each entry:
  │     binaryToHex(20-byte binary sha) → lookup objectCache[hashCache[sha]]
  │     if mode=="40000" → create_directory + recurse
  │     else → write blob bytes to file
```

---

## Helper Functions (`clone.cpp`)

| Function | Description |
|---|---|
| `readPktLine(buf, offset)` | Reads one pkt-line; returns `{flush, payload}` |
| `readRefs(buf)` | Parses full ref discovery response |
| `resolveHead(refs)` | Finds the HEAD branch name |
| `initialiseGitRepo(root, refs)` | Creates `.git/` dirs, writes HEAD and branch ref |
| `discoverRefs(curl, url)` | HTTP GET for ref discovery |
| `makeRequest(curl, request, url)` | HTTP POST for pack negotiation |
| `handlePktLine(p, payload)` | Routes pkt-line payload by phase |
| `parseRecToPktLine(p)` | Parses accumulated bytes in `p.buf` into pkt-lines |
| `writeCallbackRefDiscovery` | libcurl write callback for ref discovery |
| `writeCallbackPackFileReceive` | libcurl write callback that feeds into `parseRecToPktLine` |
| `makePktLine(payload)` | Formats a string as a pkt-line (4-hex-len prefix) |
| `compress_data(data)` | zlib deflate using `deflate()` |
| `decompress_continuous(packfile, consumedBytes, start, ...)` | zlib inflate with exact byte count tracking |
| `resolveDelta(base, delta)` | Git delta instruction interpreter |
| `parsePackFile(p, commitSha, root)` | Full packfile parser + object store writer |
| `parseTree(treeSha, prefix, hashCache, objectCache)` | Recursive tree→file writer |
| `read_be32(b)` | Read a 4-byte big-endian uint32 |
| `binaryToHex(str, 20)` | Safe SHA→hex with explicit length |

## Dependencies
- `libcurl` — HTTP transport
- `zlib` — Compression/decompression
- `openssl/sha.h` — SHA1 hashing
