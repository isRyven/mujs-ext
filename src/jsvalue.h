#ifndef js_value_h
#define js_value_h

typedef struct js_Property js_Property;
typedef struct js_Iterator js_Iterator;

/* Hint to ToPrimitive() */
enum {
	JS_HNONE,
	JS_HNUMBER,
	JS_HSTRING
};

enum js_Type {
	JS_TSHRSTR, /* type tag doubles as string zero-terminator */
	JS_TUNDEFINED,
	JS_TNULL,
	JS_TBOOLEAN,
	JS_TNUMBER,
	JS_TLITSTR, /* script literal strings */
	JS_TMEMSTR,
	JS_TOBJECT,
	JS_TCONSTSTR /* constant strings, never deallocated */
};

enum js_Class {
	JS_COBJECT,
	JS_CARRAY,
	JS_CFUNCTION,
	JS_CSCRIPT, /* function created from global/eval code */
	JS_CCFUNCTION, /* built-in function */
	JS_CERROR,
	JS_CBOOLEAN,
	JS_CNUMBER,
	JS_CSTRING,
	JS_CREGEXP,
	JS_CDATE,
	JS_CMATH,
	JS_CJSON,
	JS_CARGUMENTS,
	JS_CITERATOR,
	JS_CUSERDATA,
};

/*
	Short strings abuse the js_Value struct. By putting the type tag in the
	last byte, and using 0 as the tag for short strings, we can use the
	entire js_Value as string storage by letting the type tag serve double
	purpose as the string zero terminator.
*/

struct js_String
{
	union {
	    char shrstr[8];
        const char *ptr8;
        const uint16_t *ptr16;
    } u;
    int isunicode;
};

struct js_Value
{
	union {
		int boolean;
		double number;
		js_String string;
		js_Object *object;
	} u;
	char pad[7]; /* extra storage for shrstr */
	char type; /* type tag and zero terminator for shrstr */
};

struct js_Regexp
{
	void *prog;
	char *source;
	unsigned short flags;
	unsigned short last;
};

struct js_Object
{
	enum js_Class type;
	int extensible;
	hashtable_t *properties;
	int count; /* number of properties, for array sparseness check */
	js_Object *prototype;
	js_Object *R; /* local registry for hidden properties */
	union {
		int boolean;
		double number;
		js_String string;
		struct {
			int length;
		} a;
		struct {
			js_Function *function;
			js_Environment *scope;
		} f;
		struct {
			const char *name;
			js_CFunction function;
			js_CFunction constructor;
			int length;
		} c;
		js_Regexp r;
		struct {
			js_Object *target;
			js_Iterator *head;
		} iter;
		struct {
			const char *tag;
			void *data;
			js_HasProperty has;
			js_Put put;
			js_Delete delete;
			js_Finalize finalize;
		} user;
	} u;
	js_Object *gcnext;
	int gcmark;
};

struct js_Property
{
    js_Value value;
	const char *name;
	js_Object *getter;
	js_Object *setter;
	uint64_t hash;
	int atts;
};

struct js_Iterator
{
	const char *name;
	js_Iterator *next;
};

/* jsrun.c */
js_StringNode *jsV_newmemstring(js_State *J, const char *s, int n);
js_Value *js_tovalue(js_State *J, int idx);
void js_toprimitive(js_State *J, int idx, int hint);
js_Object *js_toobject(js_State *J, int idx);
void js_pushvalue(js_State *J, js_Value v);
void js_pushvalue2(js_State *J, js_Value *v);
void js_pushobject(js_State *J, js_Object *v);

/* jsvalue.c */
int jsV_toboolean(js_State *J, js_Value *v);
double jsV_tonumber(js_State *J, js_Value *v);
double jsV_tointeger(js_State *J, js_Value *v);
const char *jsV_tostring(js_State *J, js_Value *v);
js_Object *jsV_toobject(js_State *J, js_Value *v);
void jsV_toprimitive(js_State *J, js_Value *v, int preferred);

const char *js_itoa(char buf[32], int a);
double js_stringtofloat(const char *s, char **ep);
int jsV_numbertointeger(double n);
int jsV_numbertoint32(double n);
unsigned int jsV_numbertouint32(double n);
short jsV_numbertoint16(double n);
unsigned short jsV_numbertouint16(double n);
const char *jsV_numbertostring(js_State *J, char buf[32], double number);
double jsV_stringtonumber(js_State *J, const char *string);

/* jsproperty.c */
js_Object *jsV_newobject(js_State *J, enum js_Class type, js_Object *prototype);
js_Property *jsV_getownproperty(js_State *J, js_Object *obj, const char *name);
js_Property *jsV_getpropertyx(js_State *J, js_Object *obj, const char *name, int *own);
js_Property *jsV_getproperty(js_State *J, js_Object *obj, const char *name);
js_Property *jsV_setproperty(js_State *J, js_Object *obj, const char *name);
js_Property *jsV_nextproperty(js_State *J, js_Object *obj, const char *name);
void jsV_delproperty(js_State *J, js_Object *obj, const char *name);

js_Object *jsV_newiterator(js_State *J, js_Object *obj, int own);
const char *jsV_nextiterator(js_State *J, js_Object *iter);

void jsV_resizearray(js_State *J, js_Object *obj, int newlen);

/* jsdump.c */
void js_dumpobject(js_State *J, js_Object *obj);
void js_dumpvalue(js_State *J, js_Value v);

void js_newstringfrom(js_State *J, int idx);
int jsV_getstrlen(js_State *J, js_Value *v);
int jsV_getstrsize(js_State *J, js_Value *v);

const char* jsV_resolvetypename(js_State *J, js_Value *value, const char* keyName);

typedef enum {
	BF_NOOP,
	BF_FUNCDECL,
	BF_FUNCMETA,
	BF_FUNCNUMS,
	BF_FUNCSTRS,
	BF_FUNCVARS,
	BF_FUNCCODE,
	BF_FUNCFUNS 
} binfuncseg_t;

js_Buffer js_dumpfuncbin(js_State *J, js_Function *F, int flags);
js_Function *jsV_newfunction(js_State *J);

#define jsV_isdefined(v) (v->type != JS_TUNDEFINED)
#define jsV_isundefined(v) (v->type == JS_TUNDEFINED)
#define jsV_isnull(v) (v->type == JS_TNULL)
#define jsV_isboolean(v) (v->type == JS_TBOOLEAN)
#define jsV_isnumber(v) (v->type == JS_TNUMBER)
#define jsV_isstring(v) (v->type == JS_TSHRSTR || v->type == JS_TLITSTR || v->type == JS_TMEMSTR || v->type == JS_TCONSTSTR)
#define jsV_isprimitive(v) (v->type != JS_TOBJECT)
#define jsV_isobject(v) (v->type == JS_TOBJECT)
#define jsV_iscoercible(v) (v->type != JS_TUNDEFINED && v->type != JS_TNULL)

#endif
