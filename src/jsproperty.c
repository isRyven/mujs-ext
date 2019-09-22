#include "jsi.h"
#include "jsvalue.h"

static js_Property *newproperty(js_State *J, js_Object *obj, const char *name)
{
	js_Property node; 
	node.name = js_intern(J, name); 
	node.hash = jsU_tostrhash(name);
	node.atts = 0;
	node.value.type = JS_TUNDEFINED;
	node.value.u.number = 0;
	node.getter = NULL;
	node.setter = NULL;
	js_Property *prop = hashtable_insert(obj->properties, node.hash, &node);
	++obj->count;
	return prop;
}

static js_Property *addproperty(js_State *J, js_Object *obj, const char *name)
{
	js_Property *property = hashtable_find(obj->properties, jsU_tostrhash(name));
	if (property)
		return property;
	return newproperty(J, obj, name);
}

static void freeproperty(js_State *J, js_Object *obj, uint64_t hash)
{
	hashtable_remove(obj->properties, hash);
	--obj->count;
}

js_Object *jsV_newobject(js_State *J, enum js_Class type, js_Object *prototype)
{
	js_Object *obj = js_malloc(J, sizeof *obj);
	memset(obj, 0, sizeof *obj);
	obj->gcmark = 0;
	obj->gcnext = J->gcobj;
	J->gcobj = obj;
	++J->gccounter;

	obj->type = type;
	obj->prototype = prototype;
	obj->extensible = 1;
	obj->properties = js_malloc(J, sizeof(hashtable_t));
	hashtable_init(obj->properties, sizeof(js_Property), 8, 0);
	return obj;
}

js_Property *jsV_getownproperty(js_State *J, js_Object *obj, const char *name)
{
	return (js_Property*)hashtable_find(obj->properties, jsU_tostrhash(name));
}

js_Property *jsV_getpropertyx(js_State *J, js_Object *obj, const char *name, int *own)
{
	uint64_t hash = jsU_tostrhash(name);
	*own = 1;
	do {
		js_Property *ref = (js_Property*)hashtable_find(obj->properties, hash);
		if (ref)
			return ref;
		obj = obj->prototype;
		*own = 0;
	} while (obj);
	return NULL;
}

js_Property *jsV_getproperty(js_State *J, js_Object *obj, const char *name)
{
	uint64_t hash = jsU_tostrhash(name);
	do {
		js_Property *ref = (js_Property*)hashtable_find(obj->properties, hash);
		if (ref)
			return ref;
		obj = obj->prototype;
	} while (obj);
	return NULL;
}

static js_Property *jsV_getenumproperty(js_State *J, js_Object *obj, const char *name)
{
	uint64_t hash = jsU_tostrhash(name);
	do {
		js_Property *ref = (js_Property*)hashtable_find(obj->properties, hash);
		if (ref && !(ref->atts & JS_DONTENUM))
			return ref;
		obj = obj->prototype;
	} while (obj);
	return NULL;
}

js_Property *jsV_setproperty(js_State *J, js_Object *obj, const char *name)
{
	if (!obj->extensible) {
		js_Property *property = (js_Property*)hashtable_find(obj->properties, jsU_tostrhash(name));
		if (J->strict && !property)
			js_typeerror(J, "object is non-extensible");
		return property;
	}
	return addproperty(J, obj, name);
}

void jsV_delproperty(js_State *J, js_Object *obj, const char *name)
{
	freeproperty(J, obj, jsU_tostrhash(name));
}

/* Flatten hierarchy of enumerable properties into an iterator object */
static js_Iterator *itwalk(js_State *J, js_Iterator *iter, js_Object *obj, js_Object *seen)
{
	hashtable_foreach(js_Property, ref, obj->properties) {
		if (!(ref->atts & JS_DONTENUM)) {
			if (!seen || !jsV_getenumproperty(J, seen, ref->name)) {
				js_Iterator *head = js_malloc(J, sizeof *head);
				head->name = ref->name;
				head->next = iter;
				iter = head;
			}
		}
	}
	return iter;
}

static js_Iterator *itflatten(js_State *J, js_Object *obj)
{
	js_Iterator *iter = NULL;
	if (obj->prototype)
		iter = itflatten(J, obj->prototype);
	if (hashtable_count(obj->properties) > 0)
		iter = itwalk(J, iter, obj, obj->prototype);
	return iter;
}

js_Object *jsV_newiterator(js_State *J, js_Object *obj, int own)
{
	js_StringNode *node;
	char buf[32];
	int k;
	js_Object *io = jsV_newobject(J, JS_CITERATOR, NULL);
	io->u.iter.target = obj;
	if (own) {
		io->u.iter.head = NULL;
		if (hashtable_count(obj->properties) > 0)
			io->u.iter.head = itwalk(J, io->u.iter.head, obj, NULL);
	} else {
		io->u.iter.head = itflatten(J, obj);
	}
	if (obj->type == JS_CSTRING) {
		node = jsU_ptrtostrnode(obj->u.string.u.ptr8);
		js_Iterator *tail = io->u.iter.head;
		if (tail)
			while (tail->next)
				tail = tail->next;
		for (k = 0; k < (int)node->length; ++k) {
			js_itoa(buf, k);
			if (!jsV_getenumproperty(J, obj, buf)) {
				js_Iterator *node = js_malloc(J, sizeof *node);
				node->name = js_intern(J, js_itoa(buf, k));
				node->next = NULL;
				if (!tail)
					io->u.iter.head = tail = node;
				else {
					tail->next = node;
					tail = node;
				}
			}
		}
	}
	return io;
}

const char *jsV_nextiterator(js_State *J, js_Object *io)
{
	js_StringNode *node;
	int k;
	if (io->type != JS_CITERATOR)
		js_typeerror(J, "not an iterator");
	while (io->u.iter.head) {
		js_Iterator *next = io->u.iter.head->next;
		const char *name = io->u.iter.head->name;
		js_free(J, io->u.iter.head);
		io->u.iter.head = next;
		if (jsV_getproperty(J, io->u.iter.target, name))
			return name;
		if (io->u.iter.target->type == JS_CSTRING) {
			node = jsU_ptrtostrnode(io->u.iter.target->u.string.u.ptr8);
			if (js_isarrayindex(J, name, &k) && k < (int)node->length)
				return name;
		}
	}
	return NULL;
}

/* Walk all the properties and delete them one by one for arrays */

void jsV_resizearray(js_State *J, js_Object *obj, int newlen)
{
	char buf[32];
	const char *s;
	int k;
	if (newlen < obj->u.a.length) {
		if (obj->u.a.length > obj->count * 2) {
			js_Object *it = jsV_newiterator(J, obj, 1);
			while ((s = jsV_nextiterator(J, it))) {
				k = jsV_numbertointeger(jsV_stringtonumber(J, s));
				if (k >= newlen && !strcmp(s, jsV_numbertostring(J, buf, k)))
					jsV_delproperty(J, obj, s);
			}
		} else {
			for (k = newlen; k < obj->u.a.length; ++k) {
				jsV_delproperty(J, obj, js_itoa(buf, k));
			}
		}
	}
	obj->u.a.length = newlen;
}
