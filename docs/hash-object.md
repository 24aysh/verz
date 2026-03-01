# `verz hash-object` — Hash a File into a Blob Object

## Usage
```bash
verz hash-object <file>          # compute SHA1, print it
verz hash-object -w <file>       # compute SHA1, write blob to .verz/objects/, print it
```

## What it does
Reads a file, computes the SHA1 hash of its git blob representation, and prints the 40-character hex hash. With `-w`, it also writes the compressed object to the object store.

## Internal Flow

```
cmd_hash_object(argc, argv)
  └── hash_object(filePath, flag)
        ├── reads file content
        └── createBlobObject(content, write=(-w flag))
              └── createGitObject("blob", content, write)
                    ├── builds: "blob <size>\0<content>"
                    ├── SHA1 hash → 40-char hex
                    └── if write: zlibCompress + writeGitObject(hash)
```

## Object Format

The blob object stored on disk is:

```
zlib_compress( "blob <content_size>\0<raw_content>" )
```

Stored at: `.verz/objects/<first2chars>/<remaining38chars>`

## Helper Functions

| Function | Location | Description |
|---|---|---|
| `createBlobObject(content, write)` | `utils.cpp` | Wraps `createGitObject("blob", ...)` |
| `createGitObject(type, content, write)` | `utils.cpp` | Builds header + content, hashes, optionally writes |
| `calcSHA1(data)` | `utils.cpp` | OpenSSL SHA1 → 40-char hex string |
| `writeGitObject(content, hash)` | `utils.cpp` | zlib-compresses and writes to `.verz/objects/` |
| `zlibCompress(data)` | `utils.cpp` | zlib deflate wrapper |
