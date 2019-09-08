* Added `String.prototype.substr` method to improve compatibility with older scripts.
* Added unescaped curly braces matching support in regexp, if it does not contain digits.
* Added `js_checknumber` and`js_checkinteger` auxiliary functions to return default number if js value is `undefined`.
* Fixed bug, where `pconstruct` haven't preserved error object.
* Added stack trace printing support to `Error.prototype.toString`.
