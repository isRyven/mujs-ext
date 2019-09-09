cmake_minimum_required(VERSION 3.12)
project(mujs C)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -pedantic -Wall -Wextra -Wno-unused-parameter -fvisibility=hidden")
	if(CMAKE_C_COMPILER_ID MATCHES "Clang")
        set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -Wunreachable-code")
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

add_library(mujs STATIC ${MUJS_C_SRC} ${MUJS_H_SRC})
target_link_libraries(mujs m)
target_include_directories(mujs PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/mujs")

add_executable(mujs-repl src/main.c)
target_link_libraries(mujs-repl mujs)
target_include_directories(mujs-repl PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/mujs")

if(DEFINED JSCOMPILER AND JSCOMPILER EQUAL 0)
	message(STATUS "JS compiler is disabled")
	target_compile_definitions(mujs PRIVATE JS_COMPILER_DISABLED)
	target_compile_definitions(mujs-repl PRIVATE JS_COMPILER_DISABLED)
	if(DEFINED JSONCOMPILER AND JSONCOMPILER EQUAL 0)
		message(STATUS "JSON compiler is disabled")
		target_compile_definitions(mujs PRIVATE JSON_PARSER_DISABLED)
		target_compile_definitions(mujs-repl PRIVATE JSON_PARSER_DISABLED)
	endif()
endif()
