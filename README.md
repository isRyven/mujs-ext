# About

__MuJS__ is a ES5 compliant Javascript interpreter, written in plain C. Its lightweight implementation (the source contains around 15000 lines of C) designed for embedding in
other software to extend them with scripting capabilities. Under 64-bit Linux, the compiled library takes 200kB if optimized for size, and 300kB if optimized for speed. It is a bytecode interpreter with a very fast mechanism to call-out to C. The default build is sandboxed with very restricted access to resources. The API (lua alike) is simple and well documented, and allows strong integration with code written in other languages.

_Note: this repository contains custom MuJS edits, original repository can be found at https://github.com/ccxvii/mujs._

# Changes

Highlighted changes:
* String performance improvements, most notably ASCII string operations are now fast.
* String allocations are now more effective.
* New string API methods to get proper length and size using `js_getstrlen`, `js_getstrsize`.
* New public API methods to ease the binding implementations.
* New local registry API to define opaque properties on objects (GC aware).
* Made `js_malloc`, `js_realloc` and `js_free` available in public API as well.
* Execute scripts with custom environmental object using `js_loadstringE` API method.
* New `js_dumpscript` API method to dump compiled script into binary format.
* Added defines to exclude JS parser and compiler from compiled library.
* New `js_swap` API method to swap values on the stack.
* New `js_copyrange` to copy range of values on the stack.
* New `js_getprototypeof` to get prototype of the object. 
* Stack and instruction sizes are increased to cope with bigger scripts.
* Stack trace is now printed on error.
* Added script exit handler, to allow graceful script exit on request.

All extensions and changes can be found in [CHANGELOG](CHANGELOG.md). 

# Documentation

API documentation is stored in the `docs` directory.

# COMPILING

Compile on linux:
```bash
$ mkdir build && cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Release
$ make
```

Use in cmake project:
```cmake
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/deps/mujs")
target_link_libraries(application mujs)
// #include <mujs/mujs.h>
```

# LICENSE

```
MuJS is Copyright 2013-2017 Artifex Software, Inc.

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

The software is provided "as is" and the author disclaims all warranties with
regard to this software including all implied warranties of merchantability
and fitness. In no event shall the author be liable for any special, direct,
indirect, or consequential damages or any damages whatsoever resulting from
loss of use, data or profits, whether in an action of contract, negligence
or other tortious action, arising out of or in connection with the use or
performance of this software.
```

```
MIT License
 
Copyright (c) 2019 isRyven<ryven.mt@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## hashtable

```
Copyright (c) 2015 Mattias Gustavsson

Permission is hereby granted, free of charge, to any person obtaining a copy of 
this software and associated documentation files (the "Software"), to deal in 
the Software without restriction, including without limitation the rights to 
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
of the Software, and to permit persons to whom the Software is furnished to do 
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
SOFTWARE.
```
