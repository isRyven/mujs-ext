cmake_minimum_required(VERSION 3.12)
project(mujs C)

option(MUJS_REPL "Build mujs repl executable" OFF)
option(MUJS_TESTS "Build mujs tests" OFF)
option(MUJS_SANADDR "Sanitize address" OFF)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -pedantic -Wall -Wextra -Wno-unused-parameter -fvisibility=hidden")
	if(CMAKE_C_COMPILER_ID MATCHES "Clang")
        set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -Wunreachable-code")
    endif()
    if (MUJS_SANADDR)
        message(STATUS "Address sanitization is enabled")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fno-omit-frame-pointer -fsanitize=address")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
    endif()
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s -ffast-math -O3")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -ggdb")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fPIC")
endif()

set(MUJS_C_SRC
	src/jsarray.c
	src/jsboolean.c
	src/jsbuiltin.c
	src/jscompile.c
	src/jsdate.c
	src/jsdtoa.c
	src/jsdump.c
	src/jserror.c
	src/jsfunction.c
	src/jsgc.c
	src/jsintern.c
	src/jslex.c
	src/jsmath.c
	src/jsnumber.c
	src/jsobject.c
	src/json.c
	src/jsparse.c
	src/jsproperty.c
	src/jsregexp.c
	src/jsrepr.c
	src/jsrun.c
	src/jsstate.c
	src/jsstring.c
	src/jsutil.c
	src/jsvalue.c
	src/regexp.c
	src/utf.c
	src/utftype.c)

set(MUJS_H_SRC
	include/mujs/mujs.h
	src/utf.h
	src/regexp.h
	src/jsbuiltin.h
	src/jscompile.h
	src/jsi.h
	src/jslex.h
	src/jsparse.h
	src/jsrun.h
	src/jsutil.h
	src/jsvalue.h
	src/hashtable.h)

# MUSJ

add_library(mujs_h INTERFACE)
target_include_directories(mujs_h INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(mujs STATIC ${MUJS_C_SRC} ${MUJS_H_SRC})
target_link_libraries(mujs m mujs_h)

if(MUJS_REPL AND (PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR))
	message(STATUS "Repl compilation is enabled")
	add_executable(mujs_repl src/main.c)
	target_link_libraries(mujs_repl mujs)
endif()

if(MUJS_TESTS AND (PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR))
	message(STATUS "Tests compilation is enabled")
    enable_testing()
    if(NOT MSVC)
        add_compile_options(-Wall -Wextra)
    endif()
	add_subdirectory(tests)
endif()
