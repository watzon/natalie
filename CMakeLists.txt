cmake_minimum_required(VERSION 3.10)
project(Natalie)

set(CMAKE_VERBOSE_MAKEFILE ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_C_FLAGS_DEBUG "-D'NAT_GC_COLLECT_DEBUG=true'")
set(CMAKE_C_FLAGS_RELEASE "-O1")

add_library(
  natalie STATIC
  src/array.c
  src/basic_object.c
  src/class.c
  src/comparable.c
  src/encoding.c
  src/env.c
  src/exception.c
  src/false_class.c
  src/file.c
  src/gc.c
  src/hash.c
  src/integer.c
  src/io.c
  src/kernel.c
  src/main_obj.c
  src/matchdata.c
  src/module.c
  src/natalie.c
  src/nil_class.c
  src/object.c
  src/proc.c
  src/process.c
  src/range.c
  src/regexp.c
  src/string.c
  src/symbol.c
  src/true_class.c
  obj/nat/array.o
  obj/nat/errno.o
  obj/nat/exception.o
  obj/nat/io.o)

target_include_directories(natalie PRIVATE include)
target_include_directories(natalie PRIVATE ext/hashmap/include)
target_include_directories(natalie PRIVATE ext/onigmo)

set_property(TARGET natalie PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET natalie PROPERTY COMPILE_FLAGS "-Wall -Wextra -Werror -Wno-unused-parameter")

add_custom_command(TARGET natalie PRE_BUILD COMMAND mkdir -p obj/nat)
add_custom_command(TARGET natalie PRE_BUILD COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin/natalie --compile-obj obj/nat/exception.o ${CMAKE_CURRENT_SOURCE_DIR}/src/exception.nat)
add_custom_command(TARGET natalie PRE_BUILD COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin/natalie --compile-obj obj/nat/io.o ${CMAKE_CURRENT_SOURCE_DIR}/src/io.nat)
add_custom_command(TARGET natalie PRE_BUILD COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin/natalie --compile-obj obj/nat/errno.o ${CMAKE_CURRENT_SOURCE_DIR}/src/errno.nat)
add_custom_command(TARGET natalie PRE_BUILD COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin/natalie --compile-obj obj/nat/array.o ${CMAKE_CURRENT_SOURCE_DIR}/src/array.nat)
