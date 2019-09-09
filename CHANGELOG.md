* Added `String.prototype.substr` method to improve compatibility with older scripts.
* Added unescaped curly braces matching support in regexp, if it does not contain digits.
* Added `js_checknumber` and`js_checkinteger` auxiliary functions to return default number if js value is `undefined`.
* Fixed bug, where `pconstruct` haven't preserved error object.
* Added stack trace printing support to `Error.prototype.toString`.
* Moved js_malloc, js_realloc, js_free to public header, to allow allocations from external bindings.
* Increased stack size to 4096 values (from 256).
* Changed instruction size to int, to allow running bigger functions.
* Added `js_newcconstructorx` to allow setting constructor's prototype.   
  Push object's prototype, push constructor's prototype (defaults to `Function.prototype`).
* Added `js_setbot` to offset stack bottom index.
* Added `js_rotnpop` to rotate and pop `n` values.
* Added `js_copyrange` to copy range of values on the stack to the top.
* Added `js_getprototypeof` to fetch prototype from the specified object.
* Added `js_callscoped` to call c function in stack isolated environment, similar to js_call, but without intermediate object.
* Added `js_seal`, `js_freeze`, `js_issealed`, `js_isfrozen` functions to change object extension abilities.
* Added `js_catch_exit`, `js_exit`, `js_atexit` to issue and handle graceful script exit requests.
* Added `js_loadstringE` to load code with predefined environment variables (push object, call func).
* Added `js_getlocalregistry`, `js_setlocalregistry`, `js_dellocalregistry` to define hidden properties in object bound registry.  
  Useful for userdatas, as an alternative to `js_ref`.
* Added `js_arefb`, `js_aunrefb` convenience facility to internally create one-way reference between objects.  
  This prevents garbage collection of referenced object, until its parent is collected.
