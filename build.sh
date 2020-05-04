#!/bin/bash

CC="clang -std=c99 -Wall"
CXX="clang++ -std=c++11 -Wall"
FLAGS="-DNDEBUG"
#FLAGS=""
LLC="llc"
LINK="llvm-link"
OPT="opt"

# time $CXX -O3 catch.cpp -c -o catch.o

$CXX $FLAGS -O2 msgpack.cpp -emit-llvm -c -o msgpack.bc
$CXX $FLAGS -O2 msgpack_test.cpp -c -o msgpack_test.o
$CXX $FLAGS -O2 msgpack_fuzz.cpp -c -o msgpack_fuzz.o

$CXX $FLAGS -O2 msgpack_codegen.cpp -emit-llvm -c -o msgpack_codegen.bc

$CC $FLAGS helloworld_msgpack.c -c -o helloworld_msgpack.o
$CC $FLAGS manykernels_msgpack.c -c -o manykernels_msgpack.o

$CXX msgpack.bc msgpack_test.o msgpack_fuzz.o catch.o helloworld_msgpack.o manykernels_msgpack.o msgpack_codegen.bc -o msgpack.exe


$CXX -DNDEBUG -O3 msgpack.cpp -emit-llvm -S -c -o msgpack.ll
$LLC msgpack.ll -o msgpack.s

$LINK msgpack.bc msgpack_codegen.bc | $OPT -O3 -o merged.bc
llvm-extract merged.bc -func nop_handle_msgpack -S -o baseline.ll
llvm-extract merged.bc -func nop_handle_msgpack_templated -func _Z14handle_msgpackI12functors_nopEPKh10byte_rangeT_ -S -o template.ll


llvm-extract merged.bc -func nop_handle_msgpack_nonested -func _Z14handle_msgpackI22functors_ignore_nestedEPKh10byte_rangeT_ -S -o nonested.ll

llvm-extract merged.bc -func apply_if_top_level_is_unsigned -func _Z14handle_msgpackI35only_apply_if_top_level_is_unsignedEPKh10byte_rangeT_ -S -o unsigned.ll

llvm-dis merged.bc

$LLC baseline.ll -o baseline.s
$LLC template.ll -o template.s

time  ./msgpack.exe
