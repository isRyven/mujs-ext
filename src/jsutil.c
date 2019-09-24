#include "jsi.h"

static inline uint16_t __bswap16(uint16_t x)
{
	return (x >> 8) | (x << 8);
}

static inline uint32_t __bswap32(uint32_t v)
{
	return ((v & 0xff000000) >> 24) | ((v & 0x00ff0000) >>  8) |
		((v & 0x0000ff00) <<  8) | ((v & 0x000000ff) << 24);
}

static inline uint64_t __bswap64(uint64_t v)
{
	return ((v & ((uint64_t)0xff << (7 * 8))) >> (7 * 8)) | 
		((v & ((uint64_t)0xff << (6 * 8))) >> (5 * 8)) | 
		((v & ((uint64_t)0xff << (5 * 8))) >> (3 * 8)) | 
		((v & ((uint64_t)0xff << (4 * 8))) >> (1 * 8)) | 
		((v & ((uint64_t)0xff << (3 * 8))) << (1 * 8)) | 
		((v & ((uint64_t)0xff << (2 * 8))) << (3 * 8)) | 
		((v & ((uint64_t)0xff << (1 * 8))) << (5 * 8)) | 
		((v & ((uint64_t)0xff << (0 * 8))) << (7 * 8));
}

// https://stackoverflow.com/a/24844826
static unsigned __power_ceil(unsigned x) 
{
	if (x <= 1) return 1;
	int power = 2;
	x--;
	while (x >>= 1) power <<= 1;
	return power;
}

typedef union { 
	int i; 
	double d; 
	float f; 
	int64_t l; 
} floatint_t;

/* Dynamically grown string buffer */

static void* js_strbuf_realloc(js_State *J, js_StringBuffer *sb, int newsize)
{
	if (!sb) {
		sb = js_malloc(J, ((int)sizeof(sb->s) >= newsize) ? (int)sizeof(*sb) : (soffsetof(js_StringBuffer, s) + newsize));
		sb->n = 0;
		sb->m = ((int)sizeof(sb->s) >= newsize) ? (int)sizeof(sb->s) : newsize;
	} else if (newsize >= sb->m) {
		int size = sb->m * 3 / 2;
		if (size > newsize)
			newsize = size;
		sb = js_realloc(J, sb, newsize + soffsetof(js_StringBuffer, s));
		sb->m = newsize;
	}
	return sb;
}

void js_putc(js_State *J, js_StringBuffer **sbp, int c)
{
	js_StringBuffer *sb = *sbp;
	if (!sb || sb->n >= sb->m) {
		sb = js_strbuf_realloc(J, sb, sb ? (sb->n + 1) : 1);
		*sbp = sb;
	}
	sb->s[sb->n++] = c;
}

void js_puts(js_State *J, js_StringBuffer **sb, const char *s)
{
	while (*s)
		js_putc(J, sb, *s++);
}

void js_putm(js_State *J, js_StringBuffer **sb, const char *s, const char *e)
{
	while (s < e)
		js_putc(J, sb, *s++);
}

void js_putb(js_State *J, js_StringBuffer **sbp, const char *data, int size)
{
	js_StringBuffer *sb = *sbp;
	if (!sb || (sb->n + size) >= sb->m) {
		sb = js_strbuf_realloc(J, sb, sb ? (sb->n + size) : size);
		*sbp = sb;
	}
	memcpy(sb->s + sb->n, data, size);
	sb->n += size;
}

/* Dynamically grown octet buffer */

#define MIN_BUF_SIZE 64

static void jsbuf_realloc(js_State *J, js_Buffer *buf, unsigned int newsize) 
{
	if (!buf->data) {
		size_t size = newsize <= MIN_BUF_SIZE ? MIN_BUF_SIZE : __power_ceil(newsize);
		buf->data = (uint8_t*)js_malloc(J, size);
		buf->n = 0;
		buf->m = size;
	} else if(newsize >= buf->m) {
		size_t size = buf->m * 3 / 2;
		if (size > newsize)
			newsize = size;
		buf->data = js_realloc(J, buf->data, newsize);
		buf->m = newsize;
	}
}

void jsbuf_init(js_State *J, js_Buffer *buf, unsigned int initsize)
{
	buf->n = 0;
	buf->m = 0;
	buf->data = NULL;
	jsbuf_realloc(J, buf, initsize);
}

void jsbuf_free(js_State *J, js_Buffer *buf) 
{
	buf->n = 0;
	buf->m = 0;
	js_free(J, buf->data);
	buf->data = NULL;
}

void jsbuf_putc(js_State *J, js_Buffer *buf, int c)
{
	if (!buf->data || buf->n >= buf->m)
		jsbuf_realloc(J, buf, buf->n + 1);
	buf->data[buf->n++] = c;
}

void jsbuf_putb(js_State *J, js_Buffer *buf, uint8_t *data, int len)
{
	if (!buf->data || buf->n + len > buf->m)
		jsbuf_realloc(J, buf, buf->n + len);
	memcpy(buf->data + buf->n, data, len);
	buf->n += len;
}

void jsbuf_puts(js_State *J, js_Buffer *buf, const char *s)
{
	while (*s)
		jsbuf_putc(J, buf, *s++);
}
void jsbuf_putsz(js_State *J, js_Buffer *buf, const char *s)
{
	while (*s)
		jsbuf_putc(J, buf, *s++);
	jsbuf_putu8(J, buf, 0);
}

void jsbuf_putm(js_State *J, js_Buffer *buf, const char *s, const char *e)
{
	while (s < e)
		jsbuf_putc(J, buf, *s++);
}

void jsbuf_puti8(js_State *J, js_Buffer *buf, int8_t n)
{
	jsbuf_putc(J, buf, (int)n);
} 

void jsbuf_putu8(js_State *J, js_Buffer *buf, uint8_t n)
{
	jsbuf_putc(J, buf, (int)n);
}

void jsbuf_puti16(js_State *J, js_Buffer *buf, int16_t n)
{
#ifdef IS_BIG_ENDIAN
	n = (int16_t)__bswap16((uint16_t)n);
#endif
	jsbuf_putb(J, buf, (uint8_t *)&n, 2);
} 

void jsbuf_putu16(js_State *J, js_Buffer *buf, uint16_t n)
{
#ifdef IS_BIG_ENDIAN
	n = __bswap16(n);
#endif
	jsbuf_putb(J, buf, (uint8_t *)&n, 2);
}

void jsbuf_puti32(js_State *J, js_Buffer *buf, int32_t n) 
{	
#ifdef IS_BIG_ENDIAN
	n = (int32_t)__bswap32((uint32_t)n);
#endif
	jsbuf_putb(J, buf, (uint8_t *)&n, 4);
}

void jsbuf_putu32(js_State *J, js_Buffer *buf, uint32_t n) 
{
#ifdef IS_BIG_ENDIAN
	n = __bswap32(n);
#endif
	jsbuf_putb(J, buf, (uint8_t *)&n, 4);
}

void jsbuf_putf32(js_State *J, js_Buffer *buf, float f) 
{
	floatint_t fi;
	fi.f = f;
#ifdef IS_BIG_ENDIAN
	fi.i = __bswap32(fi.i);
#endif
	jsbuf_putb(J, buf, (uint8_t *)&fi.i, 4);
}

void jsbuf_putf64(js_State *J, js_Buffer *buf, double d) 
{
	floatint_t fi;
	fi.d = d;
#ifdef IS_BIG_ENDIAN
	fi.l = __bswap64(fi.l);
#endif
	jsbuf_putb(J, buf, (uint8_t *)&fi.l, 8);
}

int8_t jsbuf_geti8(js_State *J, js_Buffer *buf)
{
	return buf->data[buf->n++];
}

uint8_t jsbuf_getu8(js_State *J, js_Buffer *buf)
{
	return (uint8_t)buf->data[buf->n++];
}

int16_t jsbuf_geti16(js_State *J, js_Buffer *buf)
{
	int16_t n = *(int16_t*)(buf->data + buf->n);
#ifdef IS_BIG_ENDIAN
	n = (int16_t)__bswap16((uint16_t)n);
#endif
	buf->n += 2;
	return n;
}

uint16_t jsbuf_getu16(js_State *J, js_Buffer *buf)
{
	int16_t n = *(uint16_t*)(buf->data + buf->n);
#ifdef IS_BIG_ENDIAN
	n = __bswap16(n);
#endif
	buf->n += 2;
	return n;
}

int32_t jsbuf_geti32(js_State *J, js_Buffer *buf)
{
	int32_t n = *(int32_t*)(buf->data + buf->n);
#ifdef IS_BIG_ENDIAN
	n = (int32_t)__bswap32((uint32_t)n);
#endif
	buf->n += 4;
	return n;
}

uint32_t jsbuf_getu32(js_State *J, js_Buffer *buf)
{
	uint32_t n = *(uint32_t*)(buf->data + buf->n);
#ifdef IS_BIG_ENDIAN
	n = __bswap32(n);
#endif
	buf->n += 4;
	return n;
}

float jsbuf_getf32(js_State *J, js_Buffer *buf)
{
	floatint_t fi;
	fi.i = *(int32_t*)(buf->data + buf->n);
#ifdef IS_BIG_ENDIAN
	fi.i = (int32_t)__bswap32((uint32_t)fi.i);
#endif
	buf->n += 4;
	return fi.f;
}

double jsbuf_getf64(js_State *J, js_Buffer *buf)
{
	floatint_t fi;
	fi.l = *(int64_t*)(buf->data + buf->n);
#ifdef IS_BIG_ENDIAN
	fi.l = (int64_t)__bswap64((uint64_t)fi.l);
#endif
	buf->n += 8;
	return fi.d;
}

const char* jsbuf_gets(js_State *J, js_Buffer *buf) 
{
	const char *start = (char*)buf->data + buf->n;
	while (buf->data[buf->n++]) {}
	return start;
}

uint64_t jsU_tostrhash(const char *str)
{
	uint64_t hash = 5381;
	int c;
	while ((c = *str++))
		hash = ((hash << 5) + hash) + c;
	return hash;
}
