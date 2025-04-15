#include <stdio.h>
#include <stdlib.h>
#include "emu.h"

int main(int argc, char **argv) {
        FILE *fp;
        char *content;
        unsigned int length;

        if(argc < 2) {
                printf("USAGE: %s <input binary>\n", argv[0]);
                exit(1);
        }

        fp = fopen(argv[1], "r");
        if(!fp) {
                printf("ERROR: could not open %s for reading\n", argv[1]);
                exit(0);
        }
        fseek(fp, 0L, SEEK_END);
        length = ftell(fp);
        content = calloc(1, length + 1);
        fseek(fp, 0L, SEEK_SET);
        fread(content, 1, length, fp);
        fclose(fp);

        maid_get_to_work(content, length);
}
