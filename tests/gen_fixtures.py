#!/usr/bin/env python3
"""Generate DEFLATE fixtures for the inflate unit tests.

Each case produces two files in tests/fixtures/:
  <name>.expected - the original bytes
  <name>.deflate  - raw DEFLATE stream (no zlib/gzip wrapper) of those bytes,
                    produced by zlib so the C inflate implementation is checked
                    against a reference encoder.
"""
import os
import random
import zlib

FIXTURE_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "fixtures")


def raw_deflate(data: bytes, level: int = 6, strategy: int = zlib.Z_DEFAULT_STRATEGY) -> bytes:
    comp = zlib.compressobj(level, zlib.DEFLATED, -15, 9, strategy)
    return comp.compress(data) + comp.flush()


CASES = []


def add(name, data, level=6, strategy=zlib.Z_DEFAULT_STRATEGY):
    CASES.append((name, data, level, strategy))


rng = random.Random(1951)  # fixed seed: deterministic fixtures

add("empty", b"")
add("single_byte", b"a")
add("hello", b"Hello my name is Sarthak Singh!!\n")
add("repetitive", b"abcabcabd" * 5000)                       # LZ77 back-references
add("all_bytes", bytes(range(256)) * 40)                     # every byte value
add("random_64k", bytes(rng.getrandbits(8) for _ in range(65536)))  # incompressible
add("skewed_text", (b"the quick brown fox jumps over the lazy dog. " * 3000), 9)  # dynamic Huffman
add("fixed_block", b"small fixed-huffman payload", 9, zlib.Z_FIXED)  # force fixed block
add("large_1m", (b"0123456789" * 1000 + bytes(rng.getrandbits(8) for _ in range(1000))) * 500, 9)
add("level_1", b"compression level one sample " * 2000, 1)
add("level_9", b"compression level nine sample " * 2000, 9)


def main():
    os.makedirs(FIXTURE_DIR, exist_ok=True)
    for name, data, level, strategy in CASES:
        with open(os.path.join(FIXTURE_DIR, name + ".expected"), "wb") as f:
            f.write(data)
        with open(os.path.join(FIXTURE_DIR, name + ".deflate"), "wb") as f:
            f.write(raw_deflate(data, level, strategy))
    # Truncated stream: valid prefix of a larger stream, cut off mid-block.
    # The decoder must not hang or crash on it.
    full = raw_deflate(b"truncation target " * 4000)
    with open(os.path.join(FIXTURE_DIR, "truncated.deflate"), "wb") as f:
        f.write(full[: len(full) // 3])
    print(f"wrote {len(CASES)} fixture pairs + 1 truncated stream to {FIXTURE_DIR}")


if __name__ == "__main__":
    main()
