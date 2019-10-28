## Introduction

MuJS is a library, written in clean and simple C. Being an extension library, MuJS has no notion of a main program: it only works embedded in a host client program. The host program can invoke functions to execute Javascript code, read and write Javascript variables, and register C functions to be called by Javascript.

The MuJS distribution includes a sample host program called `mujs-repl`, which uses the MuJS library to offer a standalone Javascript interpreter for interactive or batch use.

This reference manual assumes that you are already familiar with the Javascript language, in particular the type system and object prototype mechanisms.

## Basic Concepts

### Values and Types
There are six basic types in Javascript: `undefined`, `null`, `boolean`, `number`, `string` and `object`.

Each object also has a class: `object`, `array`, `function`, `userdata`, `regular expression`, etc.

Javascript can call functions written in C provided by the host program, as well as other Javascript functions.

Objects with the userdata class are provided to allow arbitrary C data to be attached to Javascript objects. A userdata object has a pointer to a block of raw memory, which is managed by the host. Userdata values cannot be created or modified in Javascript, only through the C API. This guarantees the integrity of data owned by the host program.

Custom properties on userdata objects can be implemented using getter and setter property accessor functions.

Numbers are represented using `double` precision floating point values.

Strings in the C interface are zero-terminated byte arrays in `CESU-8` encoding. `CESU-8` is a variant of `UTF-8` which encodes supplementary unicode characters as surrogate pairs. This maintains compatibility with the `UTF-16` nature of JavaScript, but requires attention when passing strings using supplementary unicode characters to and from the MuJS library. It also means that you cannot have any JavaScript strings with a zero character value in MuJS.

### Environments
Each function executes within an environment which defines which variables are accessible. This is a chain of all environment records in scope, with the global environment at the top. Each environment record in MuJS is represented as an object with the null prototype, including the global environment object.

The registry is a hidden environment record which is only accessible to C. This is where Javascript values and objects that should only be accessible to C functions may be stored.

<!-- TODO: add information about object scoped registry -->

### Error Handling
All Javascript actions start from C code in the host program calling a function from the MuJS library. Whenever an exception is thrown during the compilation or execution of Javascript, control returns to the host, which can take appropriate measures (such as printing an error message). C code can also throw exceptions by calling functions to create an error object and return control to Javascript.

Internally, MuJS uses the C longjmp facility to handle errors. A protected environment uses setjmp to set a recovery point. The try statement in Javascript creates such a recovery point, as does calling `js_dostring`, `js_dofile`, `js_ploadstring`, `js_ploadfile`, `js_pcall` and `js_pconstruct`.

When an error occurs or an exception is thrown from Javascript, it does a long jump to the most recent active recovery point.

If an error occurs outside any protected environment, MuJS first calls the panic function and then calls abort, thus exiting the host application. Your panic function can avoid this exit by never returning (for example by doing a long jump to your own recovery point outside MuJS).

<!-- TODO: document exit requests? -->

### Garbage Collection
MuJS performs automatic memory management using a basic mark-and-sweep collector. Collection is automatically triggered when enough allocations have accumulated. You can also force a collection pass from C.

Userdata objects have an associated C `finalizer` function that is called when the correspending object is freed.

### The Stack
MuJS uses a virtual stack to pass values to and from C. Each element in this stack represents a Javascript value (`null`, `number`, `string`, etc).

Whenever Javascript calls C, the called function gets a new stack. This stack initially contains the this value and any arguments passed to the function. When the C function returns, the top value on the stack is passed back to the caller as the return value.

The stack values are accessed using stack indices. Index 0 always contains the `this` value, and function arguments are index 1 and up. Negative indices count down from the top of the stack, so index -1 is the top of the index and index -2 is the one below that.

## The Application Program Interface

### State
```c
typedef struct js_State js_State;
```
The interpreter state is bundled up in the opaque `struct js_State`. This state contains the value stacks, protected environments, and environment records.

```c
js_State *js_newstate(js_Alloc alloc, void *context, int flags);
```
Create a new state using the allocator function and allocator context. Pass NULL to use the default allocator.

The available flags:
* `JS_STRICT`: compile and run code using ES5 strict mode.

```c
void js_freestate(js_State *J);
```
Destroy the state and free all dynamic memory used by the state.

### Allocator
The interpreter uses a host provided function for all memory allocation needs:
```c
typedef void *(*js_Alloc)(void *memctx, void *ptr, int size);
```
When size is zero, the allocator should behave like free and return `NULL`. When size is not zero, the allocator should behave like realloc. The allocator should return `NULL` if it cannot fulfill the request. The default allocator uses `realloc` to perform all three actions.

### Panic
```c
typedef void (*js_Panic)(js_State *J);
js_Panic js_atpanic(js_State *J, js_Panic panic);
```
Set a new panic function, and return the old one.

### Report
```c
typedef void (*js_Report)(js_State *J, const char *message);
void js_setreport(js_State *J, js_Report report);
```
Set a callback function for reporting various warnings and garbage collection statistics.

### Garbage collection
```c
js_gc(js_State *J, int report);
```
Force a garbage collection pass. If the report argument is non-zero, send a summary of garbage collection statistics to the report callback function.

### Loading and compiling scripts
A script is compiled by calling `js_loadstring` or `js_loadfile`. The result of a successful compilation is a function on the top of the stack. This function can then be executed with `js_call`.
```c
void js_loadstring(js_State *J, const char *filename, const char *source);
void js_loadfile(js_State *J, const char *filename);
```
Compile the script and push the resulting function.

```c
int js_ploadstring(js_State *J, const char *filename, const char *source);
int js_ploadfile(js_State *J, const char *filename);
```
Like `js_loadstring/js_loadfile` but in a protected environment. In case of success, return `0` with the result as a function on the stack. In case of failure, return `1` with the error object on the stack.

<!-- todo: Document js_loadstringE -->

### Calling functions
```c
void js_call(js_State *J, int n);
```
To call a function, you must use the following protocol: 
1. push the function to call onto the stack, 
2. push the this value to be used by the function, 
3. push the arguments to the function in order, 
4. finally, call `js_call` with the number of arguments pushed in step 3.

Pop the function, the this value, and all arguments; execute the function; then push the return value from the function.

```c
void js_construct(js_State *J, int n);
```
The construct function implements the `new` expression in Javascript. This is similar to `js_call`, but without pushing a this value: 
1. push the constructor function to call onto the stack, 
2. push the arguments to the constructor function in order, 
3. finally, call `js_construct` with the number of arguments pushed in step 2.

```c
int js_pcall(js_State *J, int n);
int js_pconstruct(js_State *J, int n);
```
Like `js_call` and `js_construct` but in a protected environment. In case of success, return `0` with the result on the stack. In case of failure, return `1` with the error object on the stack.

### Script helpers
There are two convenience functions for loading and executing code.

```c
int js_dostring(js_State *J, const char *source);
```
Compile and execute the script in the zero-terminated string in source argument. If any errors occur, call the report callback function and return `1`. Return `0` on success.

```c
int js_dofile(js_State *J, const char *filename);
```
Load the script from the file with the given filename, then compile and execute it. If any errors occur, call the report callback function and return `1`. Return `0` on success.

### Protected environments
The `js_try` macro pushes a new protected environment and calls `setjmp`. If it returns `1`, an error has occurred. The protected environment has been popped and the error object is located on the top of the stack.

At the end of the code you want to run in the protected environment you must call `js_endtry` in order to pop the protected environment. Note: you should not call `js_endtry` when an error has occurred and you are in the true-branch of `js_try`.

Since the macro is a wrapper around setjmp, the usual restrictions apply. Use the following example as a guide for how to use js_try:

```c
if (js_try(J)) {
	fprintf(stderr, "error: %s", js_trystring(J, -1, "Error"));
	js_pop(J, 1);
	return;
}
do_some_stuff();
js_endtry(J);
```

Most of the time you shouldn't need to worry about protected environments. The functions prefixed with `p` (`js_pcall`, `js_ploadstring`, etc) handle setting up the protected environment and return simple error codes.

### Errors
```c
void js_throw(js_State *J);
```
Pop the error object on the top of the stack and return control flow to the most recent protected environment.

```c
void js_newerror(js_State *J, const char *message);
void js_newevalerror(js_State *J, const char *message);
void js_newrangeerror(js_State *J, const char *message);
void js_newreferenceerror(js_State *J, const char *message);
void js_newsyntaxerror(js_State *J, const char *message);
void js_newtypeerror(js_State *J, const char *message);
void js_newurierror(js_State *J, const char *message);
```
Push a new error object on the stack.

```c
void js_error(js_State *J, const char *fmt, ...);
void js_evalerror(js_State *J, const char *fmt, ...);
void js_rangeerror(js_State *J, const char *fmt, ...);
void js_referenceerror(js_State *J, const char *fmt, ...);
void js_syntaxerror(js_State *J, const char *fmt, ...);
void js_typeerror(js_State *J, const char *fmt, ...);
void js_urierror(js_State *J, const char *fmt, ...);
```
Wrapper to push a new error object on the stack using a printf formatting string and call js_throw.

### Stack manipulation
```c
int js_gettop(js_State *J);
void js_pop(js_State *J, int n);
void js_rot(js_State *J, int n);
void js_copy(js_State *J, int idx);
void js_remove(js_State *J, int idx);
void js_insert(js_State *J, int idx);
void js_replace(js_State* J, int idx);
```
### Comparisons and arithmetic
```c
void js_concat(js_State *J);
int js_compare(js_State *J, int *okay);
int js_equal(js_State *J);
int js_strictequal(js_State *J);
int js_instanceof(js_State *J);
```
The equivalent of the `+`, comparison, and `instanceof` operators. The okay argument to `js_compare` is set to `0` if any of the values are `NaN`, otherwise it is set to `1`.

### Primitive values
```c
void js_pushundefined(js_State *J);
void js_pushnull(js_State *J);
void js_pushboolean(js_State *J, int v);
void js_pushnumber(js_State *J, double v);
void js_pushstring(js_State *J, const char *v);
void js_pushconst(js_State *J, const char *v);
```
Push primitive values. `js_pushstring` makes a copy of the string, so it may be freed or changed after passing it in. `js_pushconst` keeps a pointer to the string, so it must not be changed or freed after passing it in.

<!-- TODO: document other string functions as well -->

```c
int js_isdefined(js_State *J, int idx);
int js_isundefined(js_State *J, int idx);
int js_isnull(js_State *J, int idx);
int js_isboolean(js_State *J, int idx);
int js_isnumber(js_State *J, int idx);
int js_isstring(js_State *J, int idx);
int js_isprimitive(js_State *J, int idx);
```
Test if a primitive value is of a given type.

```c
int js_toboolean(js_State *J, int idx);
double js_tonumber(js_State *J, int idx);
int js_tointeger(js_State *J, int idx);
int js_toint32(js_State *J, int idx);
unsigned int js_touint32(js_State *J, int idx);
short js_toint16(js_State *J, int idx);
unsigned short js_touint16(js_State *J, int idx);
const char *js_tostring(js_State *J, int idx);
```
Convert the value at the given index into a C value. If the value is an `object`, invoke the `toString` and/or `valueOf` methods to do the conversion.

_The conversion may change the actual value in the stack!_

There is no guarantee that the pointer returned by `js_tostring` will be valid after the corresponding value is removed from the stack.

Note that the `toString` and `valueOf` methods that may be invoked by these functions can throw exceptions. If you want to catch and ignore exceptions, use the following functions instead. The `error` argument is the default value that will be returned if a `toString/valueOf` method throws an exception.

```c
int js_tryboolean(js_State *J, int idx, int error);
double js_trynumber(js_State *J, int idx, double error);
int js_tryinteger(js_State *J, int idx, int error);
const char *js_trystring(js_State *J, int idx, const char *error);
```

<!-- TODO: document js_check functions -->

### Objects
```c
enum {
	JS_REGEXP_G = 1,
	JS_REGEXP_I = 2,
	JS_REGEXP_M = 4,
};

void js_newobject(js_State *J);
void js_newarray(js_State *J);
void js_newboolean(js_State *J, int v);
void js_newnumber(js_State *J, double v);
void js_newstring(js_State *J, const char *v);
void js_newregexp(js_State *J, const char *pattern, int flags);
```
Create and push objects on the stack.

```c
int js_isobject(js_State *J, int idx);
int js_isarray(js_State *J, int idx);
int js_iscallable(js_State *J, int idx);
int js_isregexp(js_State *J, int idx);
```
Test the type and class of an object on the stack.

### Properties
The property functions all work on an object. If the stack slot referenced by the index does not contain an object, they will throw an error.

```c
enum {
	JS_READONLY = 1,
	JS_DONTENUM = 2,
	JS_DONTCONF = 4,
};
```
Property attribute bit-mask values.

```c
int js_hasproperty(js_State *J, int idx, const char *name);
```
If the object has a property with the given name, return `1` and push the value of the property; otherwise return `0` and leave the stack untouched.

```c
void js_getproperty(js_State *J, int idx, const char *name);
```
Push the value of the named property of the object. If the object does not have the named property, push `undefined` instead.

```c
void js_setproperty(js_State *J, int idx, const char *name);
```
Pop a value from the top of the stack and set the value of the named property of the object.

```c
void js_defproperty(js_State *J, int idx, const char *name, int atts);
```
Pop a value from the top of the stack and set the value of the named property of the object. Also define the property attributes.

```c
void js_defaccessor(js_State *J, int idx, const char *name, int atts);
```
Define the getter and setter attributes of a property on the object. Pop the two getter and setter functions from the stack. Use null instead of a function object if you want to leave any of the functions unset.

```c
void js_delproperty(js_State *J, int idx, const char *name);
```
Delete the named property from the object.

### Array properties
```c
int js_getlength(js_State *J, int idx);
void js_setlength(js_State *J, int idx, int len);
```
Wrappers to get and set the `length` property of an object.

```c
int js_hasindex(js_State *J, int idx, int i);
void js_getindex(js_State *J, int idx, int i);
void js_setindex(js_State *J, int idx, int i);
void js_delindex(js_State *J, int idx, int i);
```
These array index functions functions are simple wrappers around the equivalent property functions. They convert the numeric index to a string to use as the property name.

### Globals
```c
void js_pushglobal(js_State *J);
```
Push the object representing the global environment record.

```c
void js_getglobal(js_State *J, const char *name);
void js_setglobal(js_State *J, const char *name);
void js_defglobal(js_State *J, const char *name, int atts);
```
Wrappers around `js_pushglobal` and `js_get/set/defproperty` to read and write the values of global variables.

### C Functions
```c
void js_newcfunction(js_State *J, js_CFunction fun, const char *name, int length);
```
Push a function object wrapping a C function pointer.

The `length` argument is the number of arguments to the function. If the function is called with fewer arguments, the argument list will be padded with `undefined`.

```c
void js_newcconstructor(js_State *J,
	js_CFunction fun, js_CFunction con,
	const char *name, int length);
```
Pop the object to set as the `prototype` property for the constructor function object. Push a function object wrapping a C function pointer, allowing for separate function pointers for regular calls and `new` operator calls.

```c
void js_currentfunction(js_State *J);
```
Push the currently executing function object.

### Userdata
```c
typedef void (*js_Finalize)(js_State *J, void *data);
typedef int (*js_HasProperty)(js_State *J, void *data, const char *name);
typedef int (*js_Put)(js_State *J, void *data, const char *name);
typedef int (*js_Delete)(js_State *J, void *data, const char *name);

void js_newuserdata(js_State *J, const char *tag, void *data,
	js_Finalize finalize);

void js_newuserdatax(js_State *J, const char *tag, void *data,
	js_HasProperty has,
	js_Put put,
	js_Delete delete,
	js_Finalize finalize);
```
Pop an object from the top of the stack to use as the internal prototype property for the new object. Push a new userdata object wrapping a pointer to C memory. The userdata object is `tagged` using a _string_, to represent the type of the C memory.

The `finalize` callback, if it is not `NULL`, will be called when the object is freed by the garbage collector.

The extended function also has callback functions for overriding property accesses. If these are set, they can be used to override accesses to certain properties. Any property accesses that are not overridden will be handled as usual in the runtime. The `HasProperty` callback should push a value and return `1` if it wants to handle the property, otherwise it should do nothing and return `0`. `Put` should use a value (`idx = -1`) and return `1` if it wants to handle the property. Likewise, `Delete` should return `1` if it wants to handle the property.

```c
int js_isuserdata(js_State *J, int idx, const char *tag);
```
Test if an object is a userdata object with the given type `tag` string.

```c
void *js_touserdata(js_State *J, int idx, const char *tag);
```
Return the wrapped pointer from a userdata object. If the object is `undefined` or `null`, return `NULL`. If the object is not a userdata object with the given type tag string, throw a type error.

### Registry
The registry can be used to store references to Javascript objects accessible from C, but hidden from Javascript to prevent tampering.

```c
void js_getregistry(js_State *J, const char *name);
void js_setregistry(js_State *J, const char *name);
void js_delregistry(js_State *J, const char *name);
```
Access properties on the hidden registry object.

```c
void js_getlocalregistry(js_State *J, int idx, const char *name);
void js_setlocalregistry(js_State *J, int idx, const char *name);
void js_dellocalregistry(js_State *J, int idx, const char *name);
```
Access hidden properties on the hidden registry object belonging to a specific object.

```c
const char *js_ref(js_State *J);
```
WIP: Pop a value from the stack and store it in the registry using a new unique property name. Return the property name.

```c
void js_unref(js_State *J, const char *ref);
```
WIP: Delete the reference from the registry.
