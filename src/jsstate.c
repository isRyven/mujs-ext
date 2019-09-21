#include "jsi.h"
#include "jsparse.h"
#include "jscompile.h"
#include "jsvalue.h"
#include "jsrun.h"
#include "jsbuiltin.h"

#include <errno.h>
#include <assert.h>

static void *js_defaultalloc(void *actx, void *ptr, int size)
{
#ifndef __has_feature
#define __has_feature(x) 0
#endif
#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
	if (size == 0) {
		free(ptr);
		return NULL;
	}
#endif
	return realloc(ptr, (size_t)size);
}

static void js_defaultreport(js_State *J, const char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
}

static void js_defaultpanic(js_State *J)
{
	js_report(J, "uncaught exception");
	/* return to javascript to abort */
}

static void js_defaultexit(js_State *J, int status)
{
	/* return to javascript to abort */
}

int js_ploadstring(js_State *J, const char *filename, const char *source)
{
	if (js_try(J))
		return 1;
	js_loadstring(J, filename, source);
	js_endtry(J);
	return 0;
}

int js_ploadfile(js_State *J, const char *filename)
{
	if (js_try(J))
		return 1;
	js_loadfile(J, filename);
	js_endtry(J);
	return 0;
}

const char *js_trystring(js_State *J, int idx, const char *error)
{
	const char *s;
	if (js_try(J)) {
		js_pop(J, 1);
		return error;
	}
	s = js_tostring(J, idx);
	js_endtry(J);
	return s;
}

double js_trynumber(js_State *J, int idx, double error)
{
	double v;
	if (js_try(J)) {
		js_pop(J, 1);
		return error;
	}
	v = js_tonumber(J, idx);
	js_endtry(J);
	return v;
}

int js_tryinteger(js_State *J, int idx, int error)
{
	int v;
	if (js_try(J)) {
		js_pop(J, 1);
		return error;
	}
	v = js_tointeger(J, idx);
	js_endtry(J);
	return v;
}

int js_tryboolean(js_State *J, int idx, int error)
{
	int v;
	if (js_try(J)) {
		js_pop(J, 1);
		return error;
	}
	v = js_toboolean(J, idx);
	js_endtry(J);
	return v;
}

double js_checknumber(js_State *J, int idx, double error)
{
	double v;
	if (js_isundefined(J, idx))
		return error;
	if (js_try(J)) {
		js_pop(J, 1);
		return error;
	}
	v = js_tonumber(J, idx);
	js_endtry(J);
	return v;
}

int js_checkinteger(js_State *J, int idx, int error)
{
	int v;
	if (js_isundefined(J, idx))
		return error;
	if (js_try(J)) {
		js_pop(J, 1);
		return error;
	}
	v = js_tointeger(J, idx);
	js_endtry(J);
	return v;
}

static void js_loadstringx(js_State *J, const char *filename, const char *source, int iseval)
{
	js_Ast *P;
	js_Function *F;

	if (js_try(J)) {
		jsP_freeparse(J);
		js_throw(J);
	}

	P = jsP_parse(J, filename, source);
	F = jsC_compilescript(J, P, iseval ? J->strict : J->default_strict);
	jsP_freeparse(J);
	js_newscript(J, F, iseval ? (J->strict ? J->E : NULL) : J->GE);

	js_endtry(J);
}

void js_loadeval(js_State *J, const char *filename, const char *source)
{
	js_loadstringx(J, filename, source, 1);
}

void js_loadstring(js_State *J, const char *filename, const char *source)
{
	js_loadstringx(J, filename, source, 0);
}

void js_loadstringE(js_State *J, const char *filename, const char *source)
{
	if (!js_isobject(J, -1))
		js_typeerror(J, "expected object");

	js_Ast *P;
	js_Function *F;

	if (js_try(J)) {
		jsP_freeparse(J);
		js_throw(J);
	}

	P = jsP_parse(J, filename, source);
	F = jsC_compilescript(J, P, J->default_strict);
	jsP_freeparse(J);

	js_Environment *env = jsR_newenvironment(J, js_toobject(J, -1), J->GE);
	js_newscript(J, F, env);
	
	js_endtry(J);
}

void js_loadfile(js_State *J, const char *filename)
{
	FILE *f;
	char *s;
	int n, t;

	f = fopen(filename, "rb");
	if (!f) {
		js_error(J, "cannot open file '%s': %s", filename, strerror(errno));
	}

	if (fseek(f, 0, SEEK_END) < 0) {
		fclose(f);
		js_error(J, "cannot seek in file '%s': %s", filename, strerror(errno));
	}

	n = ftell(f);
	if (n < 0) {
		fclose(f);
		js_error(J, "cannot tell in file '%s': %s", filename, strerror(errno));
	}

	if (fseek(f, 0, SEEK_SET) < 0) {
		fclose(f);
		js_error(J, "cannot seek in file '%s': %s", filename, strerror(errno));
	}

	if (js_try(J)) {
		fclose(f);
		js_throw(J);
	}
	s = js_malloc(J, n + 1); /* add space for string terminator */
	js_endtry(J);

	t = fread(s, 1, (size_t)n, f);
	if (t != n) {
		js_free(J, s);
		fclose(f);
		js_error(J, "cannot read data from file '%s': %s", filename, strerror(errno));
	}

	s[n] = 0; /* zero-terminate string containing file data */

	if (js_try(J)) {
		js_free(J, s);
		fclose(f);
		js_throw(J);
	}

	js_loadstring(J, filename, s);

	js_free(J, s);
	fclose(f);
	js_endtry(J);
}

int js_dostring(js_State *J, const char *source)
{
	if (js_try(J)) {
		js_report(J, js_trystring(J, -1, "Error"));
		js_pop(J, 1);
		return 1;
	}
	js_loadstring(J, "[string]", source);
	js_pushundefined(J);
	js_call(J, 0);
	js_pop(J, 1);
	js_endtry(J);
	return 0;
}

int js_dofile(js_State *J, const char *filename)
{
	if (js_try(J)) {
		js_report(J, js_trystring(J, -1, "Error"));
		js_pop(J, 1);
		return 1;
	}
	js_loadfile(J, filename);
	js_pushundefined(J);
	js_call(J, 0);
	js_pop(J, 1);
	js_endtry(J);
	return 0;
}

static const char* js_loadfuncbin_string(js_State *J, js_Buffer *sb, hashtable_t *strings) 
{
	uint32_t id = jsbuf_getu16(J, sb);
	if (id == 0xFFFF) {
		const char* temps = jsbuf_gets(J, sb);
		const char *str = js_intern(J, temps);
		uint64_t addr = (uint64_t)str;
		hashtable_insert(strings, (hashtable_count(strings) + 1), &addr);
		return str;
	} else if (id == 0)
		return jsS_sentinel.string;
	return *(const char**)hashtable_find(strings, id);
}

static js_Function* js_loadfuncbin(js_State *J, js_Buffer *sb, hashtable_t *strings, int flags) 
{
	int tempi, len, i;
	js_Function *F = jsV_newfunction(J);
	if (jsbuf_geti8(J, sb) != BF_FUNCDECL)
		js_error(J, "invalid binary");
	while (sb->n < sb->m) {
		tempi = jsbuf_geti8(J, sb);
		if (tempi == BF_FUNCMETA) {
			F->name = (flags & JS_BINSTRIPDEBUG) ? "" : js_loadfuncbin_string(J, sb, strings); 
			tempi = jsbuf_geti8(J, sb);
			F->script = BITGET(tempi, 0); 
			F->lightweight = BITGET(tempi, 1); 
			F->strict = BITGET(tempi, 2); 
			F->arguments = BITGET(tempi, 3);
			F->numparams = (int)jsbuf_getu16(J, sb);
			F->filename = (flags & JS_BINSTRIPDEBUG) ? "" : js_loadfuncbin_string(J, sb, strings);
			if (!(flags & JS_BINSTRIPDEBUG))
			{
				F->line = jsbuf_geti32(J, sb);
				F->lastline = jsbuf_geti32(J, sb);
			}
		} else if (tempi == BF_FUNCNUMS) {
			len = jsbuf_geti32(J, sb);
			F->numtab = js_malloc(J, sizeof(double) * len);
			F->numlen = len;
			for (i = 0; i < len; ++i)
				F->numtab[i] = jsbuf_getf64(J, sb);
		} else if (tempi == BF_FUNCSTRS) {
			len = jsbuf_geti32(J, sb);
			F->strtab = js_malloc(J, sizeof(char*) * len);
			F->strlen = len;
			for (i = 0; i < len; ++i)
				F->strtab[i] = js_loadfuncbin_string(J, sb, strings);
		} else if (tempi == BF_FUNCVARS) {
			len = jsbuf_geti32(J, sb);
			F->vartab = js_malloc(J, sizeof(char*) * len);
			F->varlen = len;
			for (i = 0; i < len; ++i)
				F->vartab[i] = js_loadfuncbin_string(J, sb, strings);
		} else if (tempi == BF_FUNCCODE) {
			len = jsbuf_geti32(J, sb);
			F->code = js_malloc(J, sizeof(js_Instruction) * len);
			F->codelen = len;
			for (i = 0; i < len; ++i) {
				tempi = jsbuf_getu16(J, sb);
				if (tempi == 0xFFFF) 
					F->code[i] = jsbuf_geti32(J, sb);
				else 
					F->code[i] = tempi;
			}
		} else if (tempi == BF_FUNCFUNS) {
			len = jsbuf_geti32(J, sb);
			F->funtab = js_malloc(J, sizeof(js_Function*) * len);
			F->funlen = len;
			for (i = 0; i < len; ++i)
				F->funtab[i] = js_loadfuncbin(J, sb, strings, flags);
		} else if (tempi == BF_FUNCDECL) {
			sb->n--;
			break;
		} else
			js_error(J, "invalid binary");
	}
	return F;
}

static js_Function* js_loadfuncblob(js_State *J, const char *source, int length)
{
	int flags;
	js_Buffer buf;
	js_Function *F;
	hashtable_t strings;
	if (length < 8)
		js_error(J, "invalid binary");
	hashtable_init(&strings, sizeof(uint64_t), 256, NULL);
	jsbuf_init(J, &buf, length);
	jsbuf_putb(J, &buf, (uint8_t*)source, length);
	buf.n = 0; // reset cursor
	buf.m = length; // reset length for reference
	if (js_try(J))
	{
		jsbuf_free(J, &buf);
		hashtable_term(&strings);
		js_throw(J);
	}
	if (jsbuf_geti32(J, &buf) != 0x736a756d)
		js_error(J, "invalid binary");
	flags = jsbuf_geti32(J, &buf);
	F = js_loadfuncbin(J, &buf, &strings, flags);
	js_endtry(J);
	jsbuf_free(J, &buf);
	hashtable_term(&strings);
	return F;
}

void js_loadbin(js_State *J, const char *source, int length)
{
	js_Function *F = js_loadfuncblob(J, source, length);
	js_newscript(J, F, J->GE);
}

void js_loadbinE(js_State *J, const char *source, int length)
{
	if (!js_isobject(J, -1))
		js_typeerror(J, "expected object");
	js_Function *F = js_loadfuncblob(J, source, length);
	js_Environment *env = jsR_newenvironment(J, js_toobject(J, -1), J->GE);
	js_newscript(J, F, env);
}

int js_ploadbin(js_State *J, const char *source, int length)
{
	if (js_try(J))
		return 1;
	js_loadbin(J, source,  length);
	js_endtry(J);
	return 0;
}

void js_loadbinfile(js_State *J, const char *filename)
{
	FILE *f;
	char *s;
	int n, t;

	f = fopen(filename, "rb");
	if (!f) {
		js_error(J, "cannot open file '%s': %s", filename, strerror(errno));
	}

	if (fseek(f, 0, SEEK_END) < 0) {
		fclose(f);
		js_error(J, "cannot seek in file '%s': %s", filename, strerror(errno));
	}

	n = ftell(f);
	if (n < 0) {
		fclose(f);
		js_error(J, "cannot tell in file '%s': %s", filename, strerror(errno));
	}

	if (fseek(f, 0, SEEK_SET) < 0) {
		fclose(f);
		js_error(J, "cannot seek in file '%s': %s", filename, strerror(errno));
	}

	if (js_try(J)) {
		fclose(f);
		js_throw(J);
	}
	s = js_malloc(J, n);
	js_endtry(J);

	t = fread(s, 1, (size_t)n, f);
	if (t != n) {
		js_free(J, s);
		fclose(f);
		js_error(J, "cannot read data from file '%s': %s", filename, strerror(errno));
	}

	if (js_try(J)) {
		js_free(J, s);
		fclose(f);
		js_throw(J);
	}

	js_loadbin(J, s, n);

	js_free(J, s);
	fclose(f);
	js_endtry(J);
}

int js_ploadbinfile(js_State *J, const char *filename)
{
	if (js_try(J))
		return 1;
	js_loadbinfile(J, filename);
	js_endtry(J);
	return 0;
}

js_Panic js_atpanic(js_State *J, js_Panic panic)
{
	js_Panic old = J->panic;
	J->panic = panic;
	return old;
}

void js_report(js_State *J, const char *message)
{
	if (J->report)
		J->report(J, message);
}

void js_setreport(js_State *J, js_Report report)
{
	J->report = report;
}

void js_setcontext(js_State *J, void *uctx)
{
	J->uctx = uctx;
}

void *js_getcontext(js_State *J)
{
	return J->uctx;
}

void *js_saveexit(js_State *J)
{
	if (J->exitbufset)
		js_error(J, "exit buffer already set");
	J->exitbufset = 1;
	J->exitbuf.E = J->E;
	J->exitbuf.envtop = J->envtop;
	J->exitbuf.tracetop = J->tracetop;
	J->exitbuf.top = J->top;
	J->exitbuf.bot = J->bot;
	J->exitbuf.strict = J->strict;
	J->exitbuf.pc = NULL;
	return J->exitbuf.buf;
}

void js_exit(js_State *J, int status)
{
	if (J->exitbufset) {
		J->E = J->exitbuf.E;
		J->envtop = J->exitbuf.envtop;
		J->tracetop = J->exitbuf.tracetop;
		J->top = J->exitbuf.top;
		J->bot = J->exitbuf.bot;
		J->strict = J->exitbuf.strict;
		js_pushnumber(J, (double)status);
		longjmp(J->exitbuf.buf, 1);
	}
	if (J->exit)
		J->exit(J, status);
	exit(status);
}

js_State *js_newstate(js_Alloc alloc, void *actx, int flags)
{
	js_State *J;

	assert(sizeof(js_Value) == 24);
	assert(soffsetof(js_Value, type) == 23);

	if (!alloc)
		alloc = js_defaultalloc;

	J = alloc(actx, NULL, sizeof *J);
	if (!J)
		return NULL;
	memset(J, 0, sizeof(*J));
	J->actx = actx;
	J->alloc = alloc;

	if (flags & JS_STRICT)
		J->strict = J->default_strict = 1;

	J->trace[0].name = "-top-";
	J->trace[0].file = "native";
	J->trace[0].line = 0;

	J->report = js_defaultreport;
	J->panic = js_defaultpanic;
	J->exit = js_defaultexit;

	J->stack = alloc(actx, NULL, JS_STACKSIZE * sizeof *J->stack);
	if (!J->stack) {
		alloc(actx, NULL, 0);
		return NULL;
	}

	J->gcmark = 1;
	J->nextref = 0;

	J->R = jsV_newobject(J, JS_COBJECT, NULL);
	J->G = jsV_newobject(J, JS_COBJECT, NULL);
	J->E = jsR_newenvironment(J, J->G, NULL);
	J->GE = J->E;

	jsB_init(J);

	return J;
}
