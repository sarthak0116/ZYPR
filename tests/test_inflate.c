/* Unit test driver for inflate().
 *
 * Usage:
 *   test_inflate <stream.deflate> <expected>   - decompress and byte-compare
 *   test_inflate --robust <stream.deflate>     - must terminate without crash;
 *                                                any return value is accepted
 *
 * Exit code 0 = pass, 1 = fail.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../include/inflate.h"

static uint8_t *read_file(const char *path, size_t *len) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); exit(2); }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = malloc(size > 0 ? (size_t)size : 1);
    if (!buf) { perror("malloc"); exit(2); }
    if (size > 0 && fread(buf, 1, (size_t)size, f) != (size_t)size) {
        fprintf(stderr, "short read on %s\n", path);
        exit(2);
    }
    fclose(f);
    *len = (size_t)size;
    return buf;
}

int main(int argc, char *argv[]) {
    if (argc == 3 && strcmp(argv[1], "--robust") == 0) {
        size_t src_len;
        uint8_t *src = read_file(argv[2], &src_len);
        size_t cap = src_len * 64 + 1024;
        uint8_t *out = malloc(cap);
        if (!out) { perror("malloc"); return 2; }
        size_t written = 0;
        /* Truncated input: inflate may return success or failure; it must
         * simply return, without overrunning the output buffer. */
        inflate(src, src_len, out, cap, &written);
        if (written > cap) {
            fprintf(stderr, "FAIL: wrote %zu bytes into a %zu-byte buffer\n", written, cap);
            return 1;
        }
        printf("PASS: truncated stream terminated safely (written=%zu)\n", written);
        return 0;
    }

    if (argc != 3) {
        fprintf(stderr, "usage: %s <stream.deflate> <expected>\n", argv[0]);
        return 2;
    }

    size_t src_len, exp_len;
    uint8_t *src = read_file(argv[1], &src_len);
    uint8_t *expected = read_file(argv[2], &exp_len);

    uint8_t *out = malloc(exp_len > 0 ? exp_len : 1);
    if (!out) { perror("malloc"); return 2; }

    size_t written = 0;
    int ret = inflate(src, src_len, out, exp_len, &written);

    if (ret != 0) {
        fprintf(stderr, "FAIL: inflate returned %d for %s\n", ret, argv[1]);
        return 1;
    }
    if (written != exp_len) {
        fprintf(stderr, "FAIL: wrote %zu bytes, expected %zu (%s)\n", written, exp_len, argv[1]);
        return 1;
    }
    if (exp_len > 0 && memcmp(out, expected, exp_len) != 0) {
        for (size_t i = 0; i < exp_len; i++) {
            if (out[i] != expected[i]) {
                fprintf(stderr, "FAIL: first mismatch at byte %zu: got 0x%02x, expected 0x%02x (%s)\n",
                        i, out[i], expected[i], argv[1]);
                break;
            }
        }
        return 1;
    }
    printf("PASS: %s (%zu bytes round-tripped)\n", argv[1], written);
    return 0;
}
