#!/bin/bash

CC="clang -std=c99"
CXX="clang++ -std=c++11"

# time $CXX -O3 catch.cpp -c -o catch.o

$CXX -O1 msgpack.cpp -c -o msgpack.o

$CC helloworld_msgpack.c -c -o helloworld_msgpack.o

$CXX msgpack.o catch.o helloworld_msgpack.o -o msgpack.exe

./msgpack.exe

rm -f msgpack.o
