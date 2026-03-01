# Verz Internals — Objects, Files, Index, and Packfile

A deep-dive into every on-disk format used by verz.

---

## 1. The Object Store

All content in verz is stored as **objects** — immutable, content-addressed blobs of data identified by their SHA1 hash.

### Storage Path

```
.verz/objects/<first-2-hex-chars>/<remaining-38-hex-chars>
```

Example for SHA `a2beefd59223ea16000788d77e62f96bdaf23c7c`:
```
.verz/objects/a2/beefd59223ea16000788d77e62f96bdaf23c7c
```

### On-Disk Encoding

Every object file is **zlib-compressed**. The raw (pre-compression) content always has the form:

```
<type> <size>\0<raw-content>
```

| Field | Description |
|---|---|
| `<type>` | One of: `blob`, `tree`, `commit`, `tag` |
| `<size>` | Decimal byte count of `<raw-content>` |
| `\0` | A literal null byte (0x00) separating header from body |
| `<raw-content>` | The actual object data (format varies by type) |

The SHA1 is computed over the **uncompressed** full string (header + null + content), then the **compressed** form is written to disk.

---

## 2. Blob Objects

Stores raw file content. No structure inside the content — it's just bytes.

```
blob 13\0hello, world\n
```

**Header:** `blob <byte-count>\0`  
**Body:** raw file bytes

```
┌──────────────────────────────────────────────┐
│  "blob 13\0hello, world\n"                   │
│   ^^^^  ^^  ^^^^^^^^^^^^^                    │
│   type  sz  raw content                      │
└──────────────────────────────────────────────┘
         │
    zlib compress
         │
         ▼
  .verz/objects/a2/beefd...
```

---

## 3. Tree Objects

Stores a directory snapshot. The body is a **binary** sequence of entries — one per file or subdirectory — sorted by name.

### Body Format (binary, repeated per entry)

```
<mode> SP <name> NUL <20-byte-binary-sha1>
```

| Field | Bytes | Description |
|---|---|---|
| `<mode>` | variable | ASCII digits: `100644`, `100755`, or `40000` |
| ` ` (space) | 1 | Literal ASCII space |
| `<name>` | variable | Null-terminated filename (no directory separators) |
| `\0` | 1 | Null terminator for name |
| `<sha1>` | 20 | Raw binary SHA1 (not hex) |

### File Mode Values

| Mode | Meaning |
|---|---|
| `100644` | Regular (non-executable) file |
| `100755` | Executable file |
| `40000` | Subdirectory (tree) |
| `040000` | Same as above — some tools pad with leading zero |

### Example (annotated hex)

For a tree with one file `hello.txt` (sha `a2beefd...`):

```
74 72 65 65 20 37 00       → "tree 7\0"  (header)
31 30 30 36 34 34 20       → "100644 "   (mode + space)
68 65 6c 6c 6f 2e 74 78 74 → "hello.txt" (name)
00                         → null terminator
a2 be ef d5 92 ...         → 20-byte binary SHA1
```

### Full Object String (before compression)

```
tree 29\0100644 hello.txt\0<20 binary bytes>
```

---

## 4. Commit Objects

Stores a point-in-time snapshot: which tree, who made it, and the parent commit.

### Body Format (plain text)

```
tree <40-char-hex-sha>\n
parent <40-char-hex-sha>\n         ← omitted for the first commit
author <name> <email> <ts> <tz>\n
committer <name> <email> <ts> <tz>\n
\n
<message>\n
```

### Example

```
tree 41f4c7755e270a7ebf14f7d8860dddfb7e570abc
parent 89982e3daeaeb4eb676adaa0d651bd24eab2b491
author Alice <alice@example.com> 1700000000 +0530
committer Alice <alice@example.com> 1700000000 +0530

initial commit
```

### Field Details

| Field | Format | Source |
|---|---|---|
| `tree` | 40-char hex SHA | `write_tree()` return value |
| `parent` | 40-char hex SHA | Current HEAD commit (empty for first commit) |
| Author timestamp | Unix epoch (seconds) | `std::time(nullptr)` |
| Timezone | `+HHMM` / `-HHMM` | `std::strftime(%z)` |
| Email | wrapped in `<>` | Read from `.verz/user/config` |

---

## 5. The Index File (`.verz/index`)

The staging area — a flat, plain-text file listing all files staged for the next commit.

### Format

One entry per line:
```
<mode> <sha_hex> <repo-relative-path>
```

### Example

```
100644 5761abcfdf0c26a75374c945dfe366eaeee04285 .gitignore
100644 a2beefd59223ea16000788d77e62f96bdaf23c7c README.md
100755 979a1cd784949e8bbee5d848f6abf820e27562b3 scripts/build.sh
100644 abc1234def5678901234567890abcdef01234567 src/main.cpp
```

### Lifecycle

```
verz add .       → entries written/updated
verz commit      → entries consumed, file truncated to empty
```

Each time `verz add` runs, it:
1. Reads the existing index into memory
2. Updates (or inserts) entries for each new file
3. Sorts by path
4. Overwrites the file

### Notes
- Entries are sorted alphabetically by path.
- If a file is re-staged, its SHA and mode are updated in place.
- The index does **not** store the file content — only the SHA (the blob is written to `.verz/objects/`).

---

## 6. HEAD and Branch Refs

### `.verz/HEAD`

Always contains a **symbolic ref** pointing to the current branch:
```
ref: refs/heads/main
```

On a detached HEAD (not currently supported in verz), it would contain a bare SHA instead.

### `.verz/refs/heads/<branch>`

Each branch is a plain text file containing the 40-char hex SHA of the tip commit:
```
89982e3daeaeb4eb676adaa0d651bd24eab2b491
```

### Resolution Chain

```
.verz/HEAD
  → "ref: refs/heads/main"
      → .verz/refs/heads/main
            → "89982e3da..."  (commit sha)
                  → .verz/objects/89/982e3da...
                        → (zlib-decompressed commit object)
```

---

## 7. User Config File (`.verz/user/config`)

Plain text, two lines:
```
<username>\n
<email>\n
```

Example:
```
Alice
alice@example.com
```

Written by `verz register`, read by `verz commit` (via `getUserName()` / `getUserEmail()`).

---

## 8. Git Packfile Format (received during `verz clone`)

When cloning, Git sends objects packed into a single **packfile** instead of individual object files. Verz receives this via the Smart HTTP protocol.

### Overall Structure

```
┌─────────────┬───────────┬──────────────┬──────────────┬─────┬──────────┐
│ "PACK" magic│  Version  │ Object count │  Object 1    │ ... │ SHA1     │
│  4 bytes    │  4 bytes  │   4 bytes    │  (variable)  │     │ 20 bytes │
└─────────────┴───────────┴──────────────┴──────────────┴─────┴──────────┘
  big-endian throughout
```

### Object Header (Variable-Length Integer)

Each object starts with a **VLI-encoded** header byte:

```
Byte 0:
  bit 7   → MSB: 1 = more bytes follow, 0 = last byte
  bits 6-4 → object type (3 bits)
  bits 3-0 → size[3:0] (low 4 bits of uncompressed size)

Continuation bytes (if MSB was set):
  bit 7   → MSB: 1 = more bytes follow
  bits 6-0 → next 7 bits of size
```

**Decoding example:**
```
Byte 0: 0xB2 = 1011 0010
  MSB=1 (more follows), type=(101)=5? No — type=(011)=3 (blob), size[3:0]=0010
Byte 1: 0x03 = 0000 0011
  MSB=0 (last), size[10:4] = 000 0011
→ type=3 (blob), uncompressed_size = (0b0000011 << 4) | 0b0010 = 0x32 = 50 bytes
```

### Object Types

| Code | Name | Description |
|---|---|---|
| 1 | `commit` | Commit object |
| 2 | `tree` | Tree object |
| 3 | `blob` | File content |
| 4 | `tag` | Tag object |
| 6 | `ofs_delta` | Delta against object at a negative byte offset in the same packfile |
| 7 | `ref_delta` | Delta against an object identified by its 20-byte SHA |

### OFS-Delta Extra Header

Immediately after the object VLI header, for type 6:
```
Negative offset VLI (base is at: current_object_offset - decoded_offset)
  Byte: MSB=1→more, lower 7 bits = data; each continuation: offset = (offset+1)<<7 | (b & 0x7F)
```

### REF-Delta Extra Header

For type 7: a raw **20-byte binary SHA** of the base object follows the VLI header.

### Compressed Data

After the object header (and any delta-specific bytes), the object data is **zlib-deflated**. `decompress_continuous()` tracks how many input bytes were consumed (`blobStream.total_in`) so the parser knows where the next object starts.

---

## 9. Delta Encoding (`resolveDelta`)

Deltas are binary diff instructions applied to a **base object** to produce a **target object**.

### Delta Stream Structure

```
┌────────────────────────────────────────────────────┐
│ VLI: expected source size (must == base.size())    │
│ VLI: expected target size                          │
│ Instructions (repeated until end of delta stream): │
│   if MSB=1 → COPY instruction                      │
│   if MSB=0 → INSERT instruction                    │
└────────────────────────────────────────────────────┘
```

### VLI in Delta (different from packfile header)

Each size integer is encoded as:
```
byte: MSB=1 → more follows; lower 7 bits → data (LSB first)
```

### COPY Instruction (`opcode & 0x80 != 0`)

```
Opcode bits:
  bit 0 → copyOffset byte 0 present
  bit 1 → copyOffset byte 1 present
  bit 2 → copyOffset byte 2 present
  bit 3 → copyOffset byte 3 present
  bit 4 → copySize byte 0 present
  bit 5 → copySize byte 1 present
  bit 6 → copySize byte 2 present

copyOffset: base[0] | base[1]<<8 | base[2]<<16 | base[3]<<24
copySize:   size[0] | size[1]<<8 | size[2]<<16
  If copySize == 0 → treat as 0x10000 (65536)

Action: append base[copyOffset .. copyOffset+copySize] to target
```

### INSERT Instruction (`opcode & 0x80 == 0`)

```
insertSize = opcode & 0x7F
Action: copy next insertSize bytes from the delta stream to target
```

---

## 10. Pkt-Line Protocol (Smart HTTP)

Used to frame messages between client and server during `clone`.

### Format

```
<4-hex-len><payload>
```

- `<4-hex-len>` = total length of the packet including the 4-byte length prefix itself, as 4 ASCII hex digits
- Minimum `len` = 4 (header only, empty payload)
- `len = 0000` (flush packet) signals end of a section

### Example

Payload `"want abc123\n"` (12 bytes):
```
len = 12 + 4 = 16 = 0x0010
packet = "0010want abc123\n"
```

Flush packet:
```
"0000"
```

### Multiplexed Side-Band

After NAK acknowledgement, packfile data is wrapped in a side-band channel:

```
<pkt-line with band byte prefix>
  band byte 0x01 → packfile data
  band byte 0x02 → progress / info message
  band byte 0x03 → fatal error from server
```
