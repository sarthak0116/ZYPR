#ifndef INFLATE_H
#define INFLATE_H

#include "inflate.h" 
#include <stdint.h>
#include <stdio.h>

int inflate(const uint8_t *src, size_t src_len,
            uint8_t *out, size_t out_size, size_t *out_written);

#endif