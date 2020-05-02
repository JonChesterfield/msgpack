#!/bin/bash

CC="clang -std=c99 -Wall"
CXX="clang++ -std=c++11 -Wall"
LLC="llc"
LINK="llvm-link"
OPT="opt"

# time $CXX -O3 catch.cpp -c -o catch.o

$CXX -O2 msgpack.cpp -emit-llvm -c -o msgpack.bc
$CXX -O2 msgpack_test.cpp -c -o msgpack_test.o
$CXX -O2 msgpack_fuzz.cpp -c -o msgpack_fuzz.o

$CXX -O2 msgpack_codegen.cpp -emit-llvm -c -o msgpack_codegen.bc

$CC helloworld_msgpack.c -c -o helloworld_msgpack.o
$CC manykernels_msgpack.c -c -o manykernels_msgpack.o

$CXX msgpack.bc msgpack_test.o msgpack_fuzz.o catch.o helloworld_msgpack.o manykernels_msgpack.o msgpack_codegen.bc -o msgpack.exe


$CXX -DNDEBUG -O3 msgpack.cpp -emit-llvm -S -c -o msgpack.ll
$LLC msgpack.ll -o msgpack.s

$LINK msgpack.bc msgpack_codegen.bc | $OPT -O3 | llvm-extract -func nop_handle_msgpack -func nop_handle_msgpack_templated -func _Z14handle_msgpackI12functors_nopEPKh10byte_rangeT_ -S -o codegen.ll
$LLC codegen.ll -o codegen.s

time  ./msgpack.exe
