# NOZIP
`nozip` is a tiny C library used to uncompress deflated ZIP files.

## Usage
Use in cmake project:

```cmake
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/deps/nozip")
add_executable(app main.c $<TARGET_OBJECTS:nozip>)
target_include_directories(app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/deps/nozip/src")
// #include "nozip.h"
```
