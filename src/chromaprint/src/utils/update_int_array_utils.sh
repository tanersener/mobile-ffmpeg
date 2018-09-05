#!/bin/sh

dir=$(dirname $0)

python $dir/gen_bit_reader.py 3 >$dir/unpack_int3_array.h
python $dir/gen_bit_reader.py 5 >$dir/unpack_int5_array.h

python $dir/gen_bit_writer.py 3 >$dir/ununpack_int3_array.h
python $dir/gen_bit_writer.py 5 >$dir/ununpack_int5_array.h
