
#ifndef ZIP_H
#define ZIP_H

#include <stdint.h>
#include <stdio.h>

#define SIG_LOCAL_FILE       0x04034b50
#define SIG_CENTRAL_DIR      0x02014b50
#define SIG_END_CENTRAL_DIR  0x06054b50

#pragma pack(push, 1)
typedef struct {
    uint32_t signature;       
    uint16_t disk_number;
    uint16_t disk_with_cd;
    uint16_t cd_entries_disk;
    uint16_t cd_entries_total; 
    uint32_t cd_size;         
    uint32_t cd_offset;      
    uint16_t comment_len;
} EOCD;


typedef struct {
    uint32_t signature;           
    uint16_t version_made;
    uint16_t version_needed;
    uint16_t flags;
    uint16_t compression;         
    uint16_t mod_time;
    uint16_t mod_date;
    uint32_t crc32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t filename_len;
    uint16_t extra_len;
    uint16_t comment_len;
    uint16_t disk_start;
    uint16_t internal_attr;
    uint32_t external_attr;
    uint32_t local_header_offset; 
} CentralDirHeader;

typedef struct {
    uint32_t signature;
    uint16_t version_needed;
    uint16_t flags;
    uint16_t compression;
    uint16_t mod_time;
    uint16_t mod_date;
    uint32_t crc32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t filename_len;
    uint16_t extra_len;
} LocalFileHeader;

#pragma pack(pop)
int find_eocd(FILE *fp, EOCD *eocd);
int read_central_dir(FILE *fp, EOCD *eocd);
int extract_file(FILE *fp, CentralDirHeader *cd, const char *filename);

#endif