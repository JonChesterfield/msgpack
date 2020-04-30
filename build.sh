#!/bin/bash

CC="clang -std=c99 -Wall"
CXX="clang++ -std=c++11 -Wall"

# time $CXX -O3 catch.cpp -c -o catch.o

$CXX -O1 msgpack.cpp -c -o msgpack.o
$CXX -DNOCATCH -DNDEBUG -O3 msgpack.cpp -emit-llvm -S -c -o msgpack.ll
llc msgpack.ll -o msgpack.s

$CC helloworld_msgpack.c -c -o helloworld_msgpack.o

$CXX msgpack.o catch.o helloworld_msgpack.o -o msgpack.exe

./msgpack.exe

rm -f msgpack.o
