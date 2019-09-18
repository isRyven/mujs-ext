#ifndef NOZIP_H
#define NOZIP_H

#include <stdint.h>
#include <stdio.h>
#include <time.h>

typedef struct nozip_t nozip_t;
typedef struct nozip_entry_t nozip_entry_t;

struct nozip_entry_t {
    uint64_t uncompressed_size;
    uint64_t compressed_size;
    uint64_t local_header_offset;
    const char *filename;
    time_t mtime;
};

size_t nozip_size(FILE *fd);
size_t nozip_read(nozip_t *zip, FILE *fd);
size_t nozip_size_mem(void *data, size_t size);
size_t nozip_read_mem(nozip_t *zip, void *data, size_t length);
nozip_entry_t* nozip_entries(nozip_t *zip);
nozip_entry_t* nozip_locate(nozip_t *zip, const char *name);
size_t nozip_uncompress(nozip_t *zip, nozip_entry_t *entry, void *output, size_t output_size);

#endif // NOZIP_H