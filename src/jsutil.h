#ifndef js_util_h
#define js_util_h

/* Helpers */
#define S_EITHER_STR(a, b) ((a && a[0]) ? a : b)
#define M_MIN(a, b) (a < b ? a : b)
#define M_MAX(a, b) (a > b ? a : b)
#define M_CLAMP(v, a, b) M_MIN(M_MAX(v, a), b)
#define M_IN_RANGE(v, x, y) (v >= x && v < y)
#define M_IN_RANGEX(v, x, y) (v >= x && v <= y)
#define M_FLT_EPSILON 1.19209290E-07
#define EITHER(a, b) (a ? a : b) 
#define BITSET(s, i, val) (s |= val << i)
#define BITGET(s, i) ((s >> i) & 0x1)

#define jsU_valisstr(v) ( \
	v->type == JS_TSHRSTR || \
	v->type == JS_TMEMSTR || \
	v->type == JS_TLITSTR || \
	v->type == JS_TCONSTSTR || \
	(v->type == JS_TOBJECT && v->u.object->type == JS_CSTRING))
#define jsU_valisstru(v) ( \
		v->type == JS_TSHRSTR ? 0 : \
			(v->type == JS_TLITSTR || \
			v->type == JS_TMEMSTR ||  \
			v->type == JS_TCONSTSTR) ? v->u.string.isunicode : \
				(v->type == JS_TOBJECT && v->u.object->type == JS_CSTRING) ? \
					v->u.object->u.string.isunicode : 0)
#define jsU_ptrtostrnode(p) \
	((js_StringNode*)(p - soffsetof(js_StringNode, string)))
#define jsU_valtostrnode(v) \
	((v->type == JS_TMEMSTR || v->type == JS_TLITSTR) ? \
		jsU_ptrtostrnode(v->u.string.u.ptr8) : \
		(v->type == JS_TOBJECT && v->u.object->type == JS_CSTRING) ? \
			jsU_ptrtostrnode(v->u.object->u.string.u.ptr8) : &jsS_sentinel)
#define jsU_valtocstr(v) \
	((const char*)(v->type==JS_TSHRSTR ? v->u.string.u.shrstr : \
		(v->type == JS_TMEMSTR || v->type == JS_TLITSTR || v->type==JS_TCONSTSTR) ? \
			v->u.string.u.ptr8 : \
				(v->type == JS_TOBJECT && v->u.object->type == JS_CSTRING) ? \
					v->u.object->u.string.u.ptr8 : ""))

uint64_t jsU_tostrhash(const char *str);

/* String buffer */

typedef struct js_StringBuffer { int n, m; char s[64]; } js_StringBuffer;

void js_putc(js_State *J, js_StringBuffer **sbp, int c);
void js_puts(js_State *J, js_StringBuffer **sb, const char *s);
void js_putm(js_State *J, js_StringBuffer **sb, const char *s, const char *e);
void js_putb(js_State *J, js_StringBuffer **sbp, const char *data, int size);

/* Octet buffer */

typedef struct js_Buffer { uint32_t n, m; uint8_t *data; } js_Buffer;

void jsbuf_init(js_State *J, js_Buffer *buf, unsigned int initsize);
void jsbuf_free(js_State *J, js_Buffer *buf);
void jsbuf_putc(js_State *J, js_Buffer *buf, int c);
void jsbuf_puts(js_State *J, js_Buffer *buf, const char *s);
void jsbuf_putsz(js_State *J, js_Buffer *buf, const char *s);
void jsbuf_putm(js_State *J, js_Buffer *buf, const char *s, const char *e);
void jsbuf_putb(js_State *J, js_Buffer *buf, uint8_t *data, int len);
void jsbuf_puti8(js_State *J, js_Buffer *buf, int8_t n);
void jsbuf_putu8(js_State *J, js_Buffer *buf, uint8_t n);
void jsbuf_puti16(js_State *J, js_Buffer *buf, int16_t n);
void jsbuf_putu16(js_State *J, js_Buffer *buf, uint16_t n);
void jsbuf_puti32(js_State *J, js_Buffer *buf, int32_t n);
void jsbuf_putu32(js_State *J, js_Buffer *buf, uint32_t n);
void jsbuf_putf32(js_State *J, js_Buffer *buf, float f);
void jsbuf_putf64(js_State *J, js_Buffer *buf, double d);
int8_t jsbuf_geti8(js_State *J, js_Buffer *buf);
uint8_t jsbuf_getu8(js_State *J, js_Buffer *buf);
int16_t jsbuf_geti16(js_State *J, js_Buffer *buf);
uint16_t jsbuf_getu16(js_State *J, js_Buffer *buf);
int32_t jsbuf_geti32(js_State *J, js_Buffer *buf);
uint32_t jsbuf_getu32(js_State *J, js_Buffer *buf);
float jsbuf_getf32(js_State *J, js_Buffer *buf);
double jsbuf_getf64(js_State *J, js_Buffer *buf);
const char* jsbuf_gets(js_State *J, js_Buffer *buf);


#endif
