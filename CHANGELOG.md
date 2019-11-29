* Added `String.prototype.substr` method to improve compatibility with older scripts.
* Added unescaped curly braces matching support in regexp, if it does not contain digits.
* Added `js_checknumber` and`js_checkinteger` auxiliary functions to return default number if js value is `undefined`.
* Fixed bug, where `pconstruct` haven't preserved error object.
* Added stack trace printing support to `Error.prototype.toString`.
* Moved js_malloc, js_realloc, js_free to public header, to allow allocations from external bindings.
* Increased stack size to 4096 values (from 256).
* Changed instruction size to int, to allow running bigger functions.
* Added `js_newcconstructorx` to allow setting constructor's prototype. Push object's prototype, push constructor's prototype (defaults to `Function.prototype`).
* Added `js_setbot` to offset stack bottom index.
* Added `js_rotnpop` to rotate and pop `n` values.
* Added `js_copyrange` to copy range of values on the stack to the top.
* Added `js_getprototypeof` to fetch prototype from the specified object.
* Added `js_callscoped` to call c function in stack isolated environment, similar to js_call, but without intermediate object.
* Added `js_seal`, `js_freeze`, `js_issealed`, `js_isfrozen` functions to change object extension abilities.
* Added `js_catch_exit`, `js_exit`, `js_atexit` to issue and handle graceful script exit requests, works similar to `js_try`.
* Added `js_loadstringE` to load code with predefined environment variables (push object, call func).
* Added `js_getlocalregistry`, `js_setlocalregistry`, `js_dellocalregistry` to define hidden properties in object bound registry. Useful for userdatas, as an alternative to `js_ref`.
* Added `js_arefb`, `js_aunrefb` convenience facility to internally create one-way reference between objects. This prevents garbage collection of referenced object, until its parent is collected.
* Increased ASCII string performance. `js_pushstring` analyzes input to catch ASCII stings to apply performance optimizations. 
* Added `js_pushstringu` function to specifically push unicode or ASCII encoded string, skipping string analyze.
* Added `js_pushconst`, `js_pushconstu`, functions to push constant strings that are never copied or deallocated by the mujs, unless it is interned by object conversion.
* Removed `js_pushliteral` from the public api, replaced by the `js_pushconst`. (_`js_pushliteral` is now meant to push interned strings only._)
* Added `js_isstringu` function to check if string is unicode encoded.
* Added `js_getstrlen` function to return proper string length taking into account its encoding.
* Added `js_getstrsize` function to return actual string size.
* Improved property look up performance. Should now also save the property order, however, indexes are not handled somehow special.
* Added `js_resolvetypename` to fetch proper instance type name from the constructor.
* Added facility to write and read octet buffer.
* Added `js_dumpscript` function to dump script function into binary format.
* Added `js_loadbin`, `js_ploadbin` functions to parse the precompiled script binary. Script function is pushed on the stack on success.
* Added `js_loadbinfile`, `js_ploadbinfile` functions to load precompiled scirpt binary from the file. 
* Added `-c` and `-f` flags to mujs executable, which can be used to precompile and load scripts.
* Converted API documentaion into markdown format.
* Reverted `f5de9d4` - was affecting precompiled scripts size in a negative way.  
* Added `-d` flag to mujs executable, which can be used to strip debug information from precompiled script, to ouput smaller binary.
* Added `js_loadbinE` to load precompiled script with predefined environment variables (push object, call func).
* Added `js_pushlstringu` function to specifically push unicode or ASCII encoded string of specified _size_.  
* Added `-DMUJS_TESTS` option to enable tests compilation.
* Added `-DMUJS_REPL` option to build mujs repl executable.
* Fixed `this` of primitive value was not converted to object when used in prototypes.
* Fixed primitives don't create intermediate objects on each `get property` request anymore. Returns primitive prototype instead.
* Fixed mem strings are no longer interned, saving on memory consumption.
* Optimized operators to speed up string operations. String comparisons and concatenations should run faster. 
* Optimized `String.prototype.concat` method, should run faster.
* Optimized error call stack concatenation, should generate call stack outputs efficiently.
* Optimized `charCodeAt` and `charAt` methods, should run faster.
* Optimized `String.prototype` methods to use string meta and yield output faster.
* Fixed `js_HasProperty` and `js_Put` property accessors are now executed in local scope similar to js cfunctions, where context object is stored at index `0` and and value is stored on index `1` (for `js_Put`).
* Added `js_swap` function to swap values on the stack.
