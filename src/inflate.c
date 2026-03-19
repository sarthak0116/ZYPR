#include<stdio.h>
#include<stdint.h>
#include "inflate.h"
#include <string.h>


#define MAX_CODES  288
#define MAX_BITS    15

typedef struct {
    int counts[MAX_BITS + 1]; 
    int symbols[MAX_CODES];  
    int min_len, max_len;
} HuffTree;
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

static void br_fill(BitReader *br, int n) {
    while (br->bits_count < n) {
        if (br->src_pos < br->src_len)
            br->bits |= (uint32_t)(br->src[br->src_pos++]) << br->bits_count;
        br->bits_count += 8;
    }
}

static uint32_t br_peek(BitReader *br, int n) {
    br_fill(br, n);
    return br->bits & ((1u << n) - 1);
}

static uint32_t br_read(BitReader *br, int n) {
    uint32_t val = br_peek(br, n);
    br->bits >>= n;
    br->bits_count -= n;
    return val;
}
static int build_tree(HuffTree *tree, const int *lengths, int n) {
    int offs[MAX_BITS + 1] = {0};
    memset(tree->counts, 0, sizeof(tree->counts));

    for (int i = 0; i < n; i++)
        if (lengths[i]) tree->counts[lengths[i]]++;

    tree->min_len = MAX_BITS + 1;
    tree->max_len = 0;
    for (int i = 1; i <= MAX_BITS; i++) {
        if (tree->counts[i]) {
            if (i < tree->min_len) tree->min_len = i;
            if (i > tree->max_len) tree->max_len = i;
        }
    }

    offs[1] = 0;
    for (int i = 1; i < MAX_BITS; i++)
        offs[i + 1] = offs[i] + tree->counts[i];

    for (int i = 0; i < n; i++)
        if (lengths[i]) tree->symbols[offs[lengths[i]]++] = i;

    return 0;
}

static int decode_sym(BitReader *br, const HuffTree *tree) {
    int code = 0, first = 0, index = 0;
    for (int len = 1; len <= tree->max_len; len++) {
        code |= (int)br_read(br, 1);
        int count = tree->counts[len];
        if (code - count < first) {
            return tree->symbols[index + (code - first)];
        }
        index += count;
        first  = (first + count) << 1;
        code <<= 1;
    }
    return -1; 
}

static const int len_base[29]  = {
    3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,
    35,43,51,59,67,83,99,115,131,163,195,227,258
};
static const int len_extra[29] = {
    0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,
    3,3,3,3,4,4,4,4,5,5,5,5,0
};
static const int dist_base[30] = {
    1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,
    257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577
};
static const int dist_extra[30] = {
    0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,
    7,7,8,8,9,9,10,10,11,11,12,12,13,13
};

int inflate(const uint8_t *src, size_t src_len,
            uint8_t *out, size_t out_size, size_t *out_written)
{
    BitReader br;
    br_init(&br, src, src_len);
    *out_written = 0;

    int bfinal = 0;
    while (!bfinal) {
        bfinal = (int)br_read(&br, 1); 
        int btype = (int)br_read(&br, 2); 

        if (btype == 0) {
            int rem = br.bits_count % 8;
            if (rem) { br.bits >>= rem; br.bits_count -= rem; }

            
            uint16_t len  = (uint16_t)br_read(&br, 16);
            uint16_t nlen = (uint16_t)br_read(&br, 16);
            (void)nlen; 

            for (int i = 0; i < len; i++) {
                if (*out_written >= out_size) return -1;
                out[(*out_written)++] = (uint8_t)br_read(&br, 8);
            }

        }
        
        else if (btype == 1 || btype == 2) {
            HuffTree lit_tree, dist_tree;

            if (btype == 1) {
                int lengths[288];
                for (int i = 0;   i <= 143; i++) lengths[i] = 8;
                for (int i = 144; i <= 255; i++) lengths[i] = 9;
                for (int i = 256; i <= 279; i++) lengths[i] = 7;
                for (int i = 280; i <= 287; i++) lengths[i] = 8;
                build_tree(&lit_tree, lengths, 288);

                int dlens[30];
                for (int i = 0; i < 30; i++) dlens[i] = 5;
                build_tree(&dist_tree, dlens, 30);
            } else {
                int hlit  = (int)br_read(&br, 5) + 257;
                int hdist = (int)br_read(&br, 5) + 1;
                int hclen = (int)br_read(&br, 4) + 4;

              
                static const int cl_order[19] = {
                    16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15
                };

                int cl_lens[19] = {0};
                for (int i = 0; i < hclen; i++)
                    cl_lens[cl_order[i]] = (int)br_read(&br, 3);

                HuffTree cl_tree;
                build_tree(&cl_tree, cl_lens, 19);

                int all_lens[288 + 32];
                int total = hlit + hdist;
                for (int i = 0; i < total; ) {
                    int sym = decode_sym(&br, &cl_tree);
                    if (sym < 16) {
                        all_lens[i++] = sym;
                    } else if (sym == 16) {
                        int rep = (int)br_read(&br, 2) + 3;
                        int last = all_lens[i-1];
                        for (int j = 0; j < rep; j++) all_lens[i++] = last;
                    } else if (sym == 17) {
                        int rep = (int)br_read(&br, 3) + 3;
                        for (int j = 0; j < rep; j++) all_lens[i++] = 0;
                    } else {
                        int rep = (int)br_read(&br, 7) + 11;
                        for (int j = 0; j < rep; j++) all_lens[i++] = 0;
                    }
                }

                build_tree(&lit_tree,  all_lens,        hlit);
                build_tree(&dist_tree, all_lens + hlit, hdist);

            }

         
            for (;;) {
                int sym = decode_sym(&br, &lit_tree);
                if (sym < 0)    return -1;
                if (sym == 256) break; 

                if (sym < 256) {

                    if (*out_written >= out_size) return -1;
                    out[(*out_written)++] = (uint8_t)sym;
                } else {
                    int li   = sym - 257;
                    int len  = len_base[li] + (int)br_read(&br, len_extra[li]);
                    int di   = decode_sym(&br, &dist_tree);
                    int dist = dist_base[di] + (int)br_read(&br, dist_extra[di]);

                    size_t start = *out_written;
                    for (int i = 0; i < len; i++) {
                        if (*out_written >= out_size) return -1;
                        out[(*out_written)++] = out[start - dist + i];
                    }
                }
            }
        }   
     else {
            return -1; 
        }
    }
    return 0;
}