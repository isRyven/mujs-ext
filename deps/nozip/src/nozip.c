#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
#include "nozip.h"

#define M_MAX(a, b) (a > b ? a : b)
#define M_MIN(a, b) (a < b ? a : b)
#define M_CLAMP(v, a, b) M_MIN(M_MAX(v, a), b)
#define soffsetof(x,y) ((int)offsetof(x,y))

typedef struct vfile_t vfile_t;
typedef struct nozip_eocd_t nozip_eocd_t;
typedef size_t (*vfile_read_t) (vfile_t *fd, void *ptr, size_t size, size_t count);
typedef size_t (*vfile_write_t) (vfile_t *fd, void *ptr, size_t size, size_t count);
typedef int (*vfile_seek_t) (vfile_t *fd, off_t offset, int whence);
// typedef size_t (*vfile_tell_t) (vfile_t *fd);

struct vfile_t {
	size_t offset;
	size_t length;
	void *stream;
	vfile_read_t read;
	vfile_write_t write;
	vfile_seek_t seek;
	// vfile_tell_t tell;
};

struct nozip_t {
	vfile_t file;
	size_t num_entries;
	char data[4];
};

struct nozip_eocd_t {
	uint16_t num_entries;
    uint32_t cdr_size;
    uint32_t cdr_offset;
};

#if defined(__GNUC__) || defined(__clang__)
#define PACK(x) x __attribute__((__packed__))
#elif defined(_MSC_VER)
#define PACK(x) x __pragma(pack(push, 1)) x __pragma(pack(pop))
#else
#error Unsupported compiler
#endif

PACK(struct local_file_header {
    uint32_t signature;
    uint16_t version_needed;
    uint16_t flags;
    uint16_t compression_method;
    uint16_t last_mod_file_time;
    uint16_t last_mod_file_date;
    uint32_t crc_32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t file_name_length;
    uint16_t extra_field_length;
});

PACK(struct central_dir_header {
    uint32_t signature;
    uint16_t version;
    uint16_t version_needed;
    uint16_t flags;
    uint16_t compression_method;
    uint16_t last_mod_file_time;
    uint16_t last_mod_file_date;
    uint32_t crc_32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t file_name_length;
    uint16_t extra_field_length;
    uint16_t file_comment_length;
    uint16_t disk_number_start;
    uint16_t internal_file_attributes;
    uint32_t external_file_attributes;
    uint32_t local_header_offset;
});

PACK(struct end_of_central_dir_record64 {
    uint32_t signature;
    uint64_t eocdr_size;
    uint16_t version;
    uint16_t version_needed;
    uint32_t disk_number;
    uint32_t cdr_disk_number;
    uint64_t disk_num_entries;
    uint64_t num_entries;
    uint64_t cdr_size;
    uint64_t cdr_offset;
});

PACK(struct end_of_central_dir_locator64 {
    uint32_t signature;
    uint32_t eocdr_disk;
    uint64_t eocdr_offset;
    uint32_t num_disks;
});

PACK(struct end_of_central_dir_record {
    uint32_t signature;
    uint16_t disk_number;
    uint16_t cdr_disk_number;
    uint16_t disk_num_entries;
    uint16_t num_entries;
    uint32_t cdr_size;
    uint32_t cdr_offset;
    uint16_t ZIP_file_comment_length;
});

static size_t vfile_read_file(vfile_t *file, void *output, size_t size, size_t count) 
{
	return fread(output, size, count, file->stream);
}

#if 0
static size_t vfile_write_file(vfile_t *file, void *input, size_t size, size_t count) 
{
	return fwrite(input, size, count, file->stream);
}
#endif

static int vfile_seek_file(vfile_t *file, off_t offset, int whence) 
{
	return fseeko(file->stream, offset, whence);
}

static size_t vfile_read_mem(vfile_t *file, void *output, size_t size, size_t count) 
{
	size_t finalsize;
	if (!file->stream) {
		errno = EBADF;
		return 0;
	}
  	finalsize = M_MIN(file->length - file->offset, count * size);
  	if (finalsize == 0)
  		return 0;
	memcpy(output, (char*)file->stream + file->offset, finalsize);
	file->offset += finalsize;
	return finalsize;
}

static size_t vfile_write_mem(vfile_t *file, void *input, size_t size, size_t count)
{
	size_t finalsize;
	if (!file->stream) {
		errno = EBADF;
		return 0;
	}
	finalsize = M_MIN(file->length - file->offset, count * size);
	if (finalsize == 0)
  		return 0;
	memcpy((char*)file->stream + file->offset, input, finalsize);
  	file->offset += finalsize;
	return finalsize;
} 

static int vfile_seek_mem(vfile_t *file, off_t offset, int whence) 
{
	off_t newOffset;
	if (!file->stream) {
		errno = EBADF;
		return -1;
	}
	if (whence == SEEK_CUR)
		newOffset = file->offset + offset;
	else if (whence == SEEK_END)
		newOffset = file->length + offset;
	else if (whence == SEEK_SET)
		newOffset = offset;
	else {
		errno = EINVAL;
		return -1;
	}
	file->offset = M_CLAMP((size_t)newOffset, 0, file->length);
	return 0;
}

static size_t generic_nozip_size(vfile_t *file, nozip_eocd_t *eocd)
{
	uint32_t signature;
    off_t offset;
    for (offset = sizeof(struct end_of_central_dir_record);; ++offset) {
        if (offset > UINT16_MAX || file->seek(file, -offset, SEEK_END) || !file->read(file, &signature, sizeof(signature), 1))
            return 0;
        if (signature == 0x06054B50)
            break;
    }

    // read end of central directory record
    struct end_of_central_dir_record eocdr;
    if (!(file->seek(file, -offset, SEEK_END) == 0 &&
          file->read(file, &eocdr, sizeof(eocdr), 1) &&
          eocdr.signature == 0x06054B50 &&
          eocdr.disk_number == 0 &&
          eocdr.cdr_disk_number == 0 &&
          eocdr.disk_num_entries == eocdr.num_entries))
        return 0;

    // check for zip64
    struct end_of_central_dir_record64 eocdr64;
    int zip64 = eocdr.num_entries == UINT16_MAX || eocdr.cdr_offset == UINT32_MAX || eocdr.cdr_size == UINT32_MAX;
    if (zip64) {
        // zip64 end of central directory locator
        struct end_of_central_dir_locator64 eocdl64;
        if (!(file->seek(file, -offset - sizeof(eocdl64), SEEK_END) == 0 &&
              file->read(file, &eocdl64, sizeof(eocdl64), 1) &&
              eocdl64.signature == 0x07064B50 &&
              eocdl64.eocdr_disk == 0 &&
              eocdl64.num_disks == 1))
            return 0;
        // zip64 end of central directory record
        if (!(file->seek(file, eocdl64.eocdr_offset, SEEK_SET) == 0 &&
              file->read(file, &eocdr64, sizeof(eocdr64), 1) &&
              eocdr64.signature == 0x06064B50 &&
              eocdr64.disk_number == 0 &&
              eocdr64.cdr_disk_number == 0 &&
              eocdr64.disk_num_entries == eocdr64.num_entries))
            return 0;
    }

    eocd->cdr_offset = zip64 ? eocdr64.cdr_offset : eocdr.cdr_offset;
    eocd->cdr_size = zip64 ? eocdr64.cdr_size : eocdr.cdr_size;
    eocd->num_entries = zip64 ? eocdr64.num_entries : eocdr.num_entries;

    size_t cdr_size = zip64 ? eocdr64.cdr_size : eocdr.cdr_size;
    return soffsetof(nozip_t, data) + cdr_size;
}

size_t nozip_size(FILE *fd)
{
	nozip_eocd_t eocd;
	vfile_t file = {0};
	file.read = vfile_read_file;
	file.seek = vfile_seek_file;
	file.stream = fd;
	return generic_nozip_size(&file, &eocd);
}

size_t nozip_size_mem(void *data, size_t size)
{
	nozip_eocd_t eocd;
	vfile_t file = {0};
	file.length = size;
	file.read = vfile_read_mem;
	file.seek = vfile_seek_mem;
	file.stream = data;
	return generic_nozip_size(&file, &eocd);
}

size_t generic_nozip_read(nozip_t *zip)
{
	vfile_t *file;
	char *strings;
	nozip_eocd_t eocd;
	struct nozip_entry_t *entries;
	if (!zip)
		return 0;
	file = &zip->file;
	generic_nozip_size(file, &eocd);

	// seek to central directory record
    if (file->seek(file, eocd.cdr_offset, SEEK_SET))
        return 0;	

    entries = (nozip_entry_t *)zip->data;
	strings = (char *)(entries + eocd.num_entries);

	for (size_t i = 0, i_end = eocd.num_entries; i < i_end; ++i) {
        // read central directory header, filename, extra field and skip comment
        struct central_dir_header cdh;
        if (!(file->read(file, &cdh, sizeof(cdh), 1) &&
              cdh.signature == 0x02014B50 &&
              file->read(file, strings, cdh.file_name_length + cdh.extra_field_length, 1) &&
              file->seek(file, cdh.file_comment_length, SEEK_CUR) == 0)) {
            return 0;
        }

        struct nozip_entry_t *entry = entries + i;
        entry->uncompressed_size = cdh.uncompressed_size;
        entry->compressed_size = cdh.compressed_size;
        entry->local_header_offset = cdh.local_header_offset;

        // find zip64 extended information extra field
        for (char *extra = strings + cdh.file_name_length; extra != strings + cdh.file_name_length + cdh.extra_field_length;) {
            uint16_t header_id;
            memcpy(&header_id, extra, sizeof(header_id));
            extra += sizeof(header_id);

            uint16_t data_size;
            memcpy(&data_size, extra, sizeof(data_size));
            extra += sizeof(data_size);

            switch (header_id) {
            case 0x0001:
                if (cdh.uncompressed_size == UINT32_MAX) {
                    memcpy(&entry->uncompressed_size, extra, sizeof(entry->uncompressed_size));
                    extra += sizeof(entry->uncompressed_size);
                }
                if (cdh.compressed_size == UINT32_MAX) {
                    memcpy(&entry->compressed_size, extra, sizeof(entry->compressed_size));
                    extra += sizeof(entry->compressed_size);
                }
                if (cdh.local_header_offset == UINT32_MAX) {
                    memcpy(&entry->local_header_offset, extra, sizeof(entry->local_header_offset));
                    extra += sizeof(entry->local_header_offset);
                }
                if (cdh.disk_number_start == UINT16_MAX) {
                    extra += sizeof(uint32_t);
                }
                break;
            default:
                extra += data_size;
                break;
            }
        }

        entry->filename = strings;
        strings += cdh.file_name_length;
        *strings++ = '\0';
        entry->mtime = mktime(&(struct tm){
            .tm_sec = (cdh.last_mod_file_time << 1) & 0x3F,
            .tm_min = (cdh.last_mod_file_time >> 5) & 0x3F,
            .tm_hour = (cdh.last_mod_file_time >> 11) & 0x1F,
            .tm_mday = cdh.last_mod_file_date & 0x1F,
            .tm_mon = ((cdh.last_mod_file_date >> 5) & 0xF) - 1,
            .tm_year = ((cdh.last_mod_file_date >> 9) & 0x7F) + 1980 - 1900,
            .tm_isdst = -1,
        });
    }

	return eocd.num_entries;
} 

size_t nozip_read(nozip_t *zip, FILE *fd)
{
	if (!zip)
		return 0;
	zip->file.offset = 0;
	zip->file.length = 0;
	zip->file.read = vfile_read_file;
	zip->file.seek = vfile_seek_file;
	zip->file.stream = fd;
	return zip->num_entries = generic_nozip_read(zip);
}

size_t nozip_read_mem(nozip_t *zip, void *data, size_t length)
{
	if (!zip)
		return 0;
	zip->file.offset = 0;
	zip->file.length = length;
	zip->file.read = vfile_read_mem;
	zip->file.seek = vfile_seek_mem;
	zip->file.stream = data;
	return zip->num_entries = generic_nozip_read(zip);
}

nozip_entry_t* nozip_entries(nozip_t *zip)
{
	return zip ? (nozip_entry_t*)zip->data : NULL;
}

nozip_entry_t* nozip_locate(nozip_t *zip, const char *name)
{
	if (!zip)
		return NULL;
	 for (size_t i = 0; i < zip->num_entries; ++i) {
        nozip_entry_t *e = (nozip_entry_t*)zip->data + i;
        if (!strcmp(name, e->filename)) {
        	return e;
        }
    }
    return NULL;
}

#ifndef NDEBUG
#define STBI_ZERROR(x) fprintf(stderr, "%s:%d: error: %s\n", __FILE__, __LINE__, x), 0
#else
#define STBI_ZERROR(x) 0
#endif

// fast-way is faster to check than jpeg huffman, but slow way is slower
#define STBI__ZFAST_BITS  9 // accelerate all cases in default tables
#define STBI__ZFAST_MASK  ((1 << STBI__ZFAST_BITS) - 1)

// zlib-style huffman encoding
// (jpegs packs from left, zlib from right, so can't share code)
struct stbi__zhuffman {
   uint16_t fast[1 << STBI__ZFAST_BITS];
   uint16_t firstcode[16];
   int maxcode[17];
   uint16_t firstsymbol[16];
   uint8_t  size[288];
   uint16_t value[288];
};

static inline int stbi__bitreverse16(int n)
{
  n = ((n & 0xAAAA) >>  1) | ((n & 0x5555) << 1);
  n = ((n & 0xCCCC) >>  2) | ((n & 0x3333) << 2);
  n = ((n & 0xF0F0) >>  4) | ((n & 0x0F0F) << 4);
  n = ((n & 0xFF00) >>  8) | ((n & 0x00FF) << 8);
  return n;
}

static inline int stbi__bit_reverse(int v, int bits)
{
   // to bit reverse n bits, reverse 16 and shift
   // e.g. 11 bits, bit reverse and shift away 5
   return stbi__bitreverse16(v) >> (16-bits);
}

static int stbi__zbuild_huffman(struct stbi__zhuffman *z, uint8_t *sizelist, int num)
{
   int i,k=0;
   int code, next_code[16], sizes[17];

   // DEFLATE spec for generating codes
   memset(sizes, 0, sizeof(sizes));
   memset(z->fast, 0, sizeof(z->fast));
   for (i=0; i < num; ++i)
      ++sizes[sizelist[i]];
   sizes[0] = 0;
   for (i=1; i < 16; ++i)
      if (sizes[i] > (1 << i))
         return STBI_ZERROR("bad sizes");
   code = 0;
   for (i=1; i < 16; ++i) {
      next_code[i] = code;
      z->firstcode[i] = (uint16_t) code;
      z->firstsymbol[i] = (uint16_t) k;
      code = (code + sizes[i]);
      if (sizes[i])
         if (code-1 >= (1 << i)) return STBI_ZERROR("bad codelengths");
      z->maxcode[i] = code << (16-i); // preshift for inner loop
      code <<= 1;
      k += sizes[i];
   }
   z->maxcode[16] = 0x10000; // sentinel
   for (i=0; i < num; ++i) {
      int s = sizelist[i];
      if (s) {
         int c = next_code[s] - z->firstcode[s] + z->firstsymbol[s];
         uint16_t fastv = (uint16_t) ((s << 9) | i);
         z->size [c] = (uint8_t     ) s;
         z->value[c] = (uint16_t) i;
         if (s <= STBI__ZFAST_BITS) {
            int j = stbi__bit_reverse(next_code[s],s);
            while (j < (1 << STBI__ZFAST_BITS)) {
               z->fast[j] = fastv;
               j += (1 << s);
            }
         }
         ++next_code[s];
      }
   }
   return 1;
}

// zlib-from-memory implementation for PNG reading
//    because PNG allows splitting the zlib stream arbitrarily,
//    and it's annoying structurally to have PNG call ZLIB call PNG,
//    we require PNG read all the IDATs and combine them into a single
//    memory buffer

typedef struct stbi__stream
{
    const uint8_t *start_in;
    const uint8_t *next_in;
    const uint8_t *end_in;

    uint8_t *start_out;
    uint8_t *next_out;
    uint8_t *end_out;

    vfile_t *cookie_in;
    vfile_t *cookie_out;

    size_t total_in;
    size_t total_out;

    int (*refill)(struct stbi__stream *);
    int (*flush)(struct stbi__stream *);

   int num_bits;
   uint32_t code_buffer;

   struct stbi__zhuffman z_length, z_distance;
} stbi__zbuf;

static int refill_zeros(struct stbi__stream *stream) {
    static const uint8_t zeros[64] = {0};
    stream->start_in = stream->next_in = zeros;
    stream->end_in = zeros + sizeof(zeros);
    return 0;
}

static int refill_stdio(struct stbi__stream *stream) {
    size_t n = stream->cookie_in->read(stream->cookie_in, (void *)stream->start_in, 1, stream->end_in - stream->start_in);
    if (n) {
        stream->next_in = stream->start_in;
        stream->end_in = stream->start_in + n;
        return 0;
    }
    return refill_zeros(stream);
}

static int flush_stdio(struct stbi__stream *stream) {
    size_t n = stream->cookie_out->write(stream->cookie_out, stream->start_out, 1, stream->next_out - stream->start_out);
    if (n) {
        stream->next_out = stream->start_out;
        return 0;
    }
    return -1;
}

static inline uint8_t stbi__zget8(struct stbi__stream *stream)
{
    if (stream->next_in == stream->end_in)
        stream->refill(stream);
    return *stream->next_in++;
}

static void stbi__fill_bits(stbi__zbuf *z)
{
   do {
      z->code_buffer |= (unsigned int) stbi__zget8(z) << z->num_bits;
      z->num_bits += 8;
   } while (z->num_bits <= 24);
}

static inline unsigned int stbi__zreceive(stbi__zbuf *z, int n)
{
   unsigned int k;
   if (z->num_bits < n) stbi__fill_bits(z);
   k = z->code_buffer & ((1 << n) - 1);
   z->code_buffer >>= n;
   z->num_bits -= n;
   return k;
}

static int stbi__zhuffman_decode_slowpath(stbi__zbuf *a, struct stbi__zhuffman *z)
{
   int b,s,k;
   // not resolved by fast table, so compute it the slow way
   // use jpeg approach, which requires MSbits at top
   k = stbi__bit_reverse(a->code_buffer, 16);
   for (s=STBI__ZFAST_BITS+1; ; ++s)
      if (k < z->maxcode[s])
         break;
   if (s == 16) return -1; // invalid code!
   // code size is s, so:
   b = (k >> (16-s)) - z->firstcode[s] + z->firstsymbol[s];
   a->code_buffer >>= s;
   a->num_bits -= s;
   return z->value[b];
}

static inline int stbi__zhuffman_decode(stbi__zbuf *a, struct stbi__zhuffman *z)
{
   int b,s;
   if (a->num_bits < 16) stbi__fill_bits(a);
   b = z->fast[a->code_buffer & STBI__ZFAST_MASK];
   if (b) {
      s = b >> 9;
      a->code_buffer >>= s;
      a->num_bits -= s;
      return b & 511;
   }
   return stbi__zhuffman_decode_slowpath(a, z);
}

static int stbi__zlength_base[31] = {
   3,4,5,6,7,8,9,10,11,13,
   15,17,19,23,27,31,35,43,51,59,
   67,83,99,115,131,163,195,227,258,0,0 };

static int stbi__zlength_extra[31]=
{ 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0,0,0 };

static int stbi__zdist_base[32] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,
257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577,0,0};

static int stbi__zdist_extra[32] =
{ 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};

static int stbi__parse_huffman_block(stbi__zbuf *a)
{
   uint8_t *zout = a->next_out;
   for(;;) {
      int z = stbi__zhuffman_decode(a, &a->z_length);
      if (z < 256) {
         if (z < 0) return STBI_ZERROR("bad huffman code"); // error in huffman codes
         if (zout == a->end_out) {
             a->next_out = zout;
             if (a->flush(a))
                 return 0;
             zout = a->next_out;
         }
         *zout++ = (char) z;
      } else {
         //uint8_t *p;
         int len,dist;
         if (z == 256) {
            a->next_out = zout;
            return 1;
         }
         z -= 257;
         len = stbi__zlength_base[z];
         if (stbi__zlength_extra[z]) len += stbi__zreceive(a, stbi__zlength_extra[z]);
         z = stbi__zhuffman_decode(a, &a->z_distance);
         if (z < 0) return STBI_ZERROR("bad huffman code");
         dist = stbi__zdist_base[z];
         if (stbi__zdist_extra[z]) dist += stbi__zreceive(a, stbi__zdist_extra[z]);

         if (len) {
            uint8_t *src = zout - dist;
            if (src < a->start_out)
                src += a->end_out - a->start_out; 
            do {
                if (src == a->end_out)
                    src = a->start_out;
                if (zout == a->end_out){
                    a->next_out = zout;
                    a->flush(a);
                    zout = a->next_out;
                }
                *zout++ = *src++;
            } while (--len);
         }
      }
   }
}

static int stbi__compute_huffman_codes(stbi__zbuf *a)
{
   static uint8_t length_dezigzag[19] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
   struct stbi__zhuffman z_codelength;
   uint8_t lencodes[286+32+137];//padding for maximum single op
   uint8_t codelength_sizes[19];
   int i,n;

   int hlit  = stbi__zreceive(a,5) + 257;
   int hdist = stbi__zreceive(a,5) + 1;
   int hclen = stbi__zreceive(a,4) + 4;

   memset(codelength_sizes, 0, sizeof(codelength_sizes));
   for (i=0; i < hclen; ++i) {
      int s = stbi__zreceive(a,3);
      codelength_sizes[length_dezigzag[i]] = (uint8_t) s;
   }
   if (!stbi__zbuild_huffman(&z_codelength, codelength_sizes, 19)) return 0;

   n = 0;
   while (n < hlit + hdist) {
      int c = stbi__zhuffman_decode(a, &z_codelength);
      if (c < 0 || c >= 19) return STBI_ZERROR("bad codelengths");
      if (c < 16)
         lencodes[n++] = (uint8_t) c;
      else if (c == 16) {
         c = stbi__zreceive(a,2)+3;
         memset(lencodes+n, lencodes[n-1], c);
         n += c;
      } else if (c == 17) {
         c = stbi__zreceive(a,3)+3;
         memset(lencodes+n, 0, c);
         n += c;
      } else {
         c = stbi__zreceive(a,7)+11;
         memset(lencodes+n, 0, c);
         n += c;
      }
   }
   if (n != hlit+hdist) return STBI_ZERROR("bad codelengths");
   if (!stbi__zbuild_huffman(&a->z_length, lencodes, hlit)) return 0;
   if (!stbi__zbuild_huffman(&a->z_distance, lencodes+hlit, hdist)) return 0;
   return 1;
}

static int stbi__parse_uncompressed_block(stbi__zbuf *a)
{
   uint8_t header[4];
   int len,nlen,k;
   if (a->num_bits & 7)
      stbi__zreceive(a, a->num_bits & 7); // discard
   // drain the bit-packed data into header
   k = 0;
   while (a->num_bits > 0) {
      header[k++] = (uint8_t) (a->code_buffer & 255); // suppress MSVC run-time check
      a->code_buffer >>= 8;
      a->num_bits -= 8;
   }
   // now fill header the normal way
   while (k < 4)
      header[k++] = stbi__zget8(a);
   len  = header[1] * 256 + header[0];
   nlen = header[3] * 256 + header[2];
   if (nlen != (len ^ 0xffff)) return STBI_ZERROR("zlib corrupt");
   do {
       size_t avail_out = a->end_out - a->next_out;
       while (avail_out-- && len--)
           *a->next_out++ = stbi__zget8(a);
       if (len > 0)
           a->flush(a);
   } while (len > 0);
   return 1;
}

// @TODO: should statically initialize these for optimal thread safety
static uint8_t stbi__zdefault_length[288], stbi__zdefault_distance[32];
static void stbi__init_zdefaults(void)
{
   int i;   // use <= to match clearly with spec
   for (i=0; i <= 143; ++i)     stbi__zdefault_length[i]   = 8;
   for (   ; i <= 255; ++i)     stbi__zdefault_length[i]   = 9;
   for (   ; i <= 279; ++i)     stbi__zdefault_length[i]   = 7;
   for (   ; i <= 287; ++i)     stbi__zdefault_length[i]   = 8;

   for (i=0; i <=  31; ++i)     stbi__zdefault_distance[i] = 5;
}

static int stb_inflate(struct stbi__stream *a)
{
   int final, type;
   a->num_bits = 0;
   a->code_buffer = 0;
   a->total_out = 0;
   do {
      final = stbi__zreceive(a,1);
      type = stbi__zreceive(a,2);
      if (type == 0) {
         if (!stbi__parse_uncompressed_block(a)) return 0;
      } else if (type == 3) {
         return 0;
      } else {
         if (type == 1) {
            // use fixed code lengths
            if (!stbi__zdefault_distance[31]) stbi__init_zdefaults();
            if (!stbi__zbuild_huffman(&a->z_length  , stbi__zdefault_length  , 288)) return 0;
            if (!stbi__zbuild_huffman(&a->z_distance, stbi__zdefault_distance,  32)) return 0;
         } else {
            if (!stbi__compute_huffman_codes(a)) return 0;
         }
         if (!stbi__parse_huffman_block(a)) return 0;
      }
   } while (!final);
   a->flush(a);
   return 1;
}

static int nozip_seek(vfile_t *file, const nozip_entry_t *entry) {
	if (!file || !entry)
		return -1;
    struct local_file_header lfh;
    return !(file->seek(file, entry->local_header_offset, SEEK_SET) == 0 &&
             file->read(file, &lfh, sizeof(lfh), 1) &&
             lfh.signature == 0x04034B50 &&
             file->seek(file, lfh.file_name_length + lfh.extra_field_length, SEEK_CUR) == 0);
}

size_t nozip_uncompress(nozip_t *zip, nozip_entry_t *entry, void *output, size_t output_size)
{
	size_t finalsize;
	vfile_t *file;
	if (!(zip && entry && output) || output_size == 0)
		return 0;
	finalsize = M_MIN(entry->uncompressed_size, output_size);
	if (finalsize == 0)
		return 0;
	file = &zip->file;
	if (!file->stream)
		return 0;
	if (nozip_seek(file, entry))
        return 0;
	if (entry->uncompressed_size == entry->compressed_size) {
        if (!file->read(file, output, entry->compressed_size, 1))
            return 0;
	} else {
		vfile_t outfile = {0};
		outfile.length = output_size;
		outfile.write = vfile_write_mem;
		outfile.stream = output;

		struct stbi__stream stream;
        memset(&stream, 0, sizeof(stream));
        uint8_t buffer[BUFSIZ];
        stream.start_in = buffer;
        stream.end_in = stream.next_in = buffer + sizeof(buffer);
        stream.cookie_in = &zip->file;
        stream.refill = refill_stdio;

        uint8_t window[1 << 15];
        stream.start_out = stream.next_out = window;
        stream.end_out = window + sizeof(window);
        stream.cookie_out = &outfile;
        stream.flush = flush_stdio;

        if (!stb_inflate(&stream))
            return 0;
	}

	return finalsize;
}
