#!/bin/bash

CC="clang -std=c99 -Wall"
CXX="clang++ -std=c++11 -Wall"
FLAGS="-DNDEBUG"
#FLAGS=""
LLC="llc"
LINK="llvm-link"
OPT="opt"

# time $CXX -O3 catch.cpp -c -o catch.o

rm -rf *.ll

$CXX $FLAGS -O2 msgpack.cpp -emit-llvm -c -o msgpack.bc
$CXX $FLAGS -O2 msgpack_test.cpp -c -o msgpack_test.o
$CXX $FLAGS -O2 msgpack_fuzz.cpp -c -o msgpack_fuzz.o

$CXX $FLAGS -O2 msgpack_codegen.cpp -emit-llvm -c -o msgpack_codegen.bc

$CC $FLAGS helloworld_msgpack.c -c -o helloworld_msgpack.o
$CC $FLAGS manykernels_msgpack.c -c -o manykernels_msgpack.o

$CXX msgpack.bc msgpack_test.o msgpack_fuzz.o catch.o helloworld_msgpack.o manykernels_msgpack.o msgpack_codegen.bc -o msgpack.exe


$CXX -DNDEBUG -O3 msgpack.cpp -emit-llvm -S -c -o msgpack.ll
$LLC msgpack.ll -o msgpack.s

$LINK msgpack.bc msgpack_codegen.bc | $OPT -internalize -internalize-public-api-list="foronly_string_example,foronly_unsigned_example,nop_handle_msgpack_example,skip_next_message_example" -O3 -o merged.bc


llvm-extract merged.bc -func foronly_unsigned_example -S -o foronly_unsigned_example.ll
llvm-extract merged.bc -func foronly_string_example -S -o foronly_string_example.ll
llvm-extract merged.bc -func skip_next_message_example -S -o skip_next_message_example.ll
llvm-extract merged.bc -func nop_handle_msgpack_example -S -o nop_handle_msgpack_example.ll


llvm-dis merged.bc
for i in *.ll; do $LLC $i; done

time  ./msgpack.exe
