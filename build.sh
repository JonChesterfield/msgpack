#!/bin/bash

CC="clang -std=c99 -Wall"
CXX="clang++ -std=c++11 -Wall"

# time $CXX -O3 catch.cpp -c -o catch.o

$CXX -O2 msgpack.cpp -c -o msgpack.o
$CXX -O2 msgpack_test.cpp -c -o msgpack_test.o
$CXX -O2 msgpack_fuzz.cpp -c -o msgpack_fuzz.o

$CC helloworld_msgpack.c -c -o helloworld_msgpack.o
$CC manykernels_msgpack.c -c -o manykernels_msgpack.o

$CXX -DNDEBUG -O3 msgpack.cpp -emit-llvm -S -c -o msgpack.ll
llc msgpack.ll -o msgpack.s


$CXX msgpack.o msgpack_test.o msgpack_fuzz.o catch.o helloworld_msgpack.o manykernels_msgpack.o -o msgpack.exe

time  ./msgpack.exe

rm -f msgpack.o
