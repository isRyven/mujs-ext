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

add_library(mujs STATIC ${MUJS_C_SRC} ${MUJS_H_SRC})
target_link_libraries(mujs m)
target_include_directories(mujs PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/mujs")

add_executable(mujs-repl src/main.c)
target_link_libraries(mujs-repl mujs)
target_include_directories(mujs-repl PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/mujs")

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
	message(STATUS "Standalone TypeScript compiler is enabled")

	add_library(muts STATIC ${MUJS_C_SRC} ${MUJS_H_SRC})
	target_link_libraries(muts m)
	target_include_directories(muts PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/mujs")
	target_compile_definitions(muts PRIVATE JS_COMPILER_DISABLED)

	add_executable(tsc src/tsc.c $<TARGET_OBJECTS:nozip> $<TARGET_OBJECTS:assets_data_img>)
	target_link_libraries(tsc muts)
	target_include_directories(tsc PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/mujs")
	target_include_directories(tsc PRIVATE "${PROJECT_SOURCE_DIR}/deps/nozip/src")

	file(GLOB_RECURSE ASSETS_DATA "${PROJECT_SOURCE_DIR}/src_ts/*" )
	list(FILTER ASSETS_DATA EXCLUDE REGEX "\\.js$")
	file(GLOB_RECURSE ASSETS_SCRIPTS "${PROJECT_SOURCE_DIR}/src_ts/*.js" )

	set(ASSETS_IMG_TEMP_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets_temp")
	set(ASSETS_IMG_PATH "${CMAKE_CURRENT_BINARY_DIR}/assets_data.pak")
	file(MAKE_DIRECTORY ${ASSETS_IMG_TEMP_DIR})

	add_custom_target(precompile_assets_scripts
	  	COMMAND mujs-repl -c ${ASSETS_SCRIPTS}
	  	DEPENDS mujs-repl ${ASSETS_SCRIPTS}
	  	WORKING_DIRECTORY ${ASSETS_IMG_TEMP_DIR}
	  	COMMENT "Precompiling assets scripts"
	)

	add_custom_target(copy_assets_data
		COMMAND ${CMAKE_COMMAND} -E copy ${ASSETS_DATA} .
		DEPENDS ${ASSETS_DATA}
		WORKING_DIRECTORY ${ASSETS_IMG_TEMP_DIR}
		COMMENT "Copying assets data"
	)

	add_custom_target(build_assets_image
		COMMAND ${CMAKE_COMMAND} -E tar "cfv" ${ASSETS_IMG_PATH} --format=zip "*"
		WORKING_DIRECTORY ${ASSETS_IMG_TEMP_DIR}
		COMMENT "Generating assets data image"
		DEPENDS precompile_assets_scripts copy_assets_data
	)

	set(ASSETS_IMG_C ${CMAKE_CURRENT_BINARY_DIR}/assets_data_img.c)
	add_custom_command(
	  	OUTPUT ${ASSETS_IMG_C}
	  	COMMAND toheader __pak_data_img ${ASSETS_IMG_PATH} ${ASSETS_IMG_C}
	  	DEPENDS toheader build_assets_image ${ASSETS_IMG_PATH} ${ASSETS_SCRIPTS} ${ASSETS_DATA}
	  	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	  	COMMENT "Generating assets image c file"
	)

	add_library(assets_data_img OBJECT ${ASSETS_IMG_C})
endif()

# DEPS

add_subdirectory(test)
add_subdirectory(deps/nozip)
add_subdirectory(deps/toheader)
