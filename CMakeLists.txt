cmake_minimum_required(VERSION 3.12)
project(mujs C)

option(TSCOMPILER "Enable TypeScript Compiler (standalone)" OFF)
option(JSCOMPILER "Include JS Parser And Compiler (disable to reduce binary size)" ON)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -D_GNU_SOURCE -pedantic -Wall -Wextra -Wno-unused-parameter -fvisibility=hidden")
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

# MUSJ

add_library(mujs_h INTERFACE)
target_include_directories(mujs_h INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(mujs STATIC ${MUJS_C_SRC} ${MUJS_H_SRC})
target_link_libraries(mujs m mujs_h)

add_executable(mujs-repl src/main.c)
target_link_libraries(mujs-repl mujs)

if(NOT JSCOMPILER)
	message(STATUS "JS compiler is disabled")
	target_compile_definitions(mujs PRIVATE JS_COMPILER_DISABLED)
	target_compile_definitions(mujs-repl PRIVATE JS_COMPILER_DISABLED)
	if(DEFINED JSONCOMPILER AND JSONCOMPILER EQUAL 0)
		message(STATUS "JSON compiler is disabled")
		target_compile_definitions(mujs PRIVATE JSON_PARSER_DISABLED)
		target_compile_definitions(mujs-repl PRIVATE JSON_PARSER_DISABLED)
	endif()
endif()

# TypeScript Compiler

if(TSCOMPILER)
	include(cmake/CMakeZRC.cmake)
	message(STATUS "Standalone TypeScript compiler is enabled")

	add_library(muts STATIC ${MUJS_C_SRC} ${MUJS_H_SRC})
	target_link_libraries(muts m mujs_h)
	target_compile_definitions(muts PRIVATE JS_COMPILER_DISABLED)

	zrc_add_resource_library(tsc_assets 
		WORKING_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src_tsc
		manifest.json
	)

	file(GLOB tsc_scripts "${PROJECT_SOURCE_DIR}/src_tsc/*.js")
	set(tsc_assets_temp "${CMAKE_CURRENT_BINARY_DIR}/__tsc_assets_temp")
	file(MAKE_DIRECTORY ${tsc_assets_temp})
	list(TRANSFORM tsc_scripts REPLACE "^${PROJECT_SOURCE_DIR}/src_tsc/(.+)" "\\1bin" OUTPUT_VARIABLE tsc_bin_scripts)
	
	zrc_add_resources(tsc_assets 
		WORKING_DIR ${tsc_assets_temp}
		${tsc_bin_scripts}
	)

	add_custom_target(tsc_precompile_scripts
	  	COMMAND mujs-repl -c ${tsc_scripts}
	  	DEPENDS mujs-repl ${tsc_scripts}
	  	WORKING_DIRECTORY ${tsc_assets_temp}
	  	COMMENT "Precompiling assets scripts"
	)

	add_dependencies(tsc_assets tsc_precompile_scripts)

	add_executable(tsc src/tsc.c)
	target_link_libraries(tsc muts tsc_assets)

endif()

add_subdirectory(test)
