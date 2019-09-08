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
