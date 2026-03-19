# Zypr 

A ZIP extractor built from scratch in C. No external libraries. Just raw bytes.

Implements the ZIP format spec and a full DEFLATE decompressor (RFC 1951) by hand — bit reader, Huffman trees, LZ77 back-references, all of it.

## Features

- Extract ZIP files from the command line
- Supports compression method 0 (stored) and method 8 (DEFLATE)
- Full inflate implementation with fixed and dynamic Huffman blocks
- Zero dependencies — just a C compiler

## Installation

### Homebrew
```bash
brew tap sarthaksingh/zypr
brew install zypr
```

### Build from source
```bash
git clone https://github.com/sarthaksingh/zypr
cd zypr
make
```

## Usage

```bash
# Extract a ZIP file
zypr archive.zip

# List contents without extracting
zypr -l archive.zip

# Extract to a specific directory
zypr -o output_dir archive.zip
```

## Project Structure

```
zypr/
├── src/
│   ├── main.c        — entry point and argument parsing
│   ├── zip.c         — ZIP format parsing and extraction
│   └── inflate.c     — DEFLATE decompressor
├── include/
│   ├── zip.h
│   └── inflate.h
└── Makefile
```

## How it works

ZIP files have three main sections:

1. **Local file headers + data** — one per file, scattered through the archive
2. **Central directory** — an index of all files, sitting near the end
3. **End of Central Directory (EOCD)** — the very last thing in the file, points to the central directory

Zypr scans backwards from the end of the file to find the EOCD, then walks the central directory to find each file, then decompresses them using a hand-written DEFLATE implementation.

## What is DEFLATE?

DEFLATE (used by most ZIP files) combines two algorithms:

- **LZ77** — replaces repeated byte sequences with back-references ("go back 20 bytes, copy 5 bytes")
- **Huffman coding** — encodes symbols with variable-length bit codes, shorter for common symbols

Zypr implements the full inflate algorithm including stored blocks, fixed Huffman blocks, and dynamic Huffman blocks.

