#!/bin/bash

CXX="clang++ -std=c++11"

# time $CXX -O3 catch.cpp -c -o catch.o

$CXX -O1 msgpack.cpp -c -o msgpack.o

$CXX msgpack.o catch.o -o msgpack.exe

./msgpack.exe

rm -f msgpack.o
