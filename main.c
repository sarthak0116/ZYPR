#include <stdio.h>
#include <stdint.h>
#include "zip.h"

int main(int argc, char *argv[]){
    if(argc < 2){
        printf("Usage: %s <file.zip>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "rb");
    if (file == NULL) {
       perror("Error opening file");
       return 1;
   }

    uint32_t sig;
    fread(&sig, 4, 1, file);

    if (sig == SIG_LOCAL_FILE) {
        printf("valid ZIP\n");
        printf("\n");
    } else {
        printf("not a ZIP\n");
        printf("\n");
    }

    EOCD eocd;
    if (find_eocd(file, &eocd) != 0) {
        printf("not a valid ZIP\n");
        printf("\n");
        fclose(file);
        return 1;
    }

    printf("total files: %d\n", eocd.cd_entries_total);
    printf("central dir offset: %u\n", eocd.cd_offset);


    read_central_dir(file, &eocd);
    fclose(file);
}

