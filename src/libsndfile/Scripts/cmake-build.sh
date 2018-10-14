#!/bin/bash -e

rm -rf CMakeCache.txt CMakeFiles/

cmake -DWerror=on -Werror=dev -Werror=deprecated .

make clean
make
make check
