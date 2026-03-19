#include<stdio.h>
#include<stdint.h>
#include "inflate.h"

typedef struct {
    const uint8_t *src;      
    size_t         src_len;  
    size_t         src_pos;  
    uint32_t       bits;     
    int            bits_count; 
} BitReader;

static void br_init(BitReader *br, const uint8_t *src, size_t len) {
    br->src         = src;
    br->src_len     = len;
    br->src_pos     = 0;
    br->bits        = 0;
    br->bits_count  = 0;
}