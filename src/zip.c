#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>  
#include "zip.h"
#include "inflate.h"   
int find_eocd(FILE *fp, EOCD *eocd){
    fseek(fp, 0, SEEK_END);  
    long size = ftell(fp); 
    
    for (long pos = size - 4; pos >= 0; pos--) {
        fseek(fp, pos, SEEK_SET); 
        uint32_t sig;
        fread(&sig, 4, 1, fp);     
        if (sig == SIG_END_CENTRAL_DIR) {
            fseek(fp, pos, SEEK_SET);      
            fread(eocd, sizeof(EOCD), 1, fp);  
            return 0;                       
        }
    }
    return -1;
}

int read_central_dir(FILE *fp, EOCD *eocd){
    fseek(fp, eocd->cd_offset, SEEK_SET);
    for (int i = 0; i < eocd->cd_entries_total; i++) {
        CentralDirHeader cd;
        fread(&cd, sizeof(CentralDirHeader), 1, fp);
        char filename[4096] = {0};
        fread(filename, 1, cd.filename_len, fp);
        if (strstr(filename, "__MACOSX")) {
            fseek(fp, cd.extra_len + cd.comment_len, SEEK_CUR);
            continue;
        }
        printf("%s\n", filename);
        fseek(fp, cd.extra_len + cd.comment_len, SEEK_CUR);

        long saved = ftell(fp);      
        extract_file(fp, &cd, filename); 
        fseek(fp, saved, SEEK_SET);     
    }
    return 0;  
}


int extract_file(FILE *fp, CentralDirHeader *cd, const char *filename){
    printf("  extracting: %s\n", filename);
    printf("  compressed size: %u\n", cd->compressed_size);
    printf("  uncompressed size: %u\n", cd->uncompressed_size);
    printf("  compression method: %u\n", cd->compression);
    fseek(fp, cd->local_header_offset, SEEK_SET);
    LocalFileHeader lh;
    fread(&lh, sizeof(LocalFileHeader), 1, fp);
    fseek(fp, lh.filename_len + lh.extra_len, SEEK_CUR);
    
    uint8_t *buf = malloc(cd->compressed_size);
    if (!buf) { perror("malloc"); return -1; }
    fread(buf, 1, cd->compressed_size, fp);

    printf("  trying to create: '%s'\n", filename);
    FILE *out = fopen(filename, "wb");
    if (!out) { perror("fopen"); free(buf); return -1; }  
    if (cd->compression == 0) {
            fwrite(buf, 1, cd->compressed_size, out);
        } else if (cd->compression == 8) {
            uint8_t *out_buf = malloc(cd->uncompressed_size);
            if (!out_buf) { perror("malloc"); fclose(out); free(buf); return -1; }
            size_t written = 0;
            int ret = inflate(buf, cd->compressed_size, out_buf, cd->uncompressed_size, &written);
            if (ret != 0) {
                fprintf(stderr, "inflate failed for %s\n", filename);
            } else {
                fwrite(out_buf, 1, written, out);
            }
            free(out_buf);
        } else {
            fprintf(stderr, "unsupported compression %d for %s\n", cd->compression, filename);
        }
        fclose(out);
        free(buf);


    return 0;
}