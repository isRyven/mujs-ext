#include "nozip.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s [-lvx] file [file ...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    int mode;
    if (!strcmp("-l", argv[1]))
        mode = 'l';
    else if (!strcmp("-v", argv[1]))
        mode = 'v';
    else if (!strcmp("-x", argv[1]))
        mode = 'x';
    else if (!strcmp("-z", argv[1]))
        mode = 'z';
    else {
        fprintf(stderr, "%s: illegal option -- %s\n", argv[0], argv[1]);
        return EXIT_FAILURE;
    }

    FILE *fp = fopen(argv[2], "rb");
    if (!fp) {
        perror(argv[2]);
        return EXIT_FAILURE;
    }


    size_t zsize = nozip_size(fp);
    nozip_t *zip = malloc(zsize); 
    size_t num_entries = nozip_read(zip, fp);

    if (num_entries == 0 || !zip) {
        perror(argv[2]);
        return EXIT_FAILURE;
    }
    nozip_entry_t *entries = nozip_entries(zip);
    switch (mode) {
    case 'l':
        for (size_t i = 0; i < num_entries; ++i) {
            printf("%s\n", (entries + i)->filename);
        }
        break;
    case 'v':
        for (size_t i = 0; i < num_entries; ++i) {
            nozip_entry_t *e = entries + i;
            char buf[32];
            strftime(buf, sizeof(buf), "%Y %b %d %H:%M", localtime(&e->mtime));
            printf("%10" PRIu64 " %10" PRIu64 " %10" PRIu64 " %s %s\n",
                   e->local_header_offset, e->compressed_size, e->uncompressed_size, buf, e->filename);
        }
        break;
    case 'x':
    case 'z':
        for (int argi = 3; argi < argc; ++argi) {
            for (size_t i = 0; i < num_entries; ++i) {
                nozip_entry_t *e = entries + i;
                if (!strcmp(argv[argi], e->filename)) {
                    void *output;
                    if (e->uncompressed_size == 0)
                        continue;
                    output = malloc(e->uncompressed_size);
                    if (!output) {
                        perror(argv[argi]);
                        return EXIT_FAILURE;
                    }
                    if (nozip_uncompress(zip, e, output, e->uncompressed_size) != e->uncompressed_size) {
                        perror(argv[argi]);
                        return EXIT_FAILURE;
                    }
                    fwrite(output, 1, e->uncompressed_size, stdout);
                    free(output);
                }
            }
        }
        break;
    }

    free(zip);
    fclose(fp);

    return 0;
}
