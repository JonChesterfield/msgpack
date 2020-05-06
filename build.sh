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
llvm-extract merged.bc -func nop_handle_msgpack_templated  -S -o template.ll


llvm-extract merged.bc -func nop_handle_msgpack_nonested -S -o nonested.ll

llvm-extract merged.bc -func apply_if_top_level_is_unsigned   -S -o unsigned.ll

llvm-extract merged.bc -func foronly_unsigned_example -func _ZN7msgpack19handle_msgpack_voidIZNS_16foronly_unsignedIPFvmEEEvNS_10byte_rangeET_E5innerEEvS4_S5_ -S -o foronly_unsigned_example.ll

llvm-extract merged.bc -func foronly_string_example -S -o foronly_string_example.ll


llvm-dis merged.bc
for i in *.ll; do $LLC $i; done

time  ./msgpack.exe
