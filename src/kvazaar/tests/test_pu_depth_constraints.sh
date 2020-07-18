#!/bin/sh

# Test pu depth constraints.

set -eu
. "${0%/*}/util.sh"

common_args='264x130 8 --preset=ultrafast --gop=8'

# Default
valgrind_test $common_args
valgrind_test $common_args --pu-depth-inter=1-3
valgrind_test $common_args --pu-depth-intra=1-3
valgrind_test $common_args --pu-depth-inter=1-3,2-3
valgrind_test $common_args --pu-depth-intra=1-3,2-3
valgrind_test $common_args --pu-depth-inter=,1-3,,,2-3,2-2
valgrind_test $common_args --pu-depth-intra=,1-3,,,2-3,2-2

# Test invalid input
encode_test 264x130 1 1 --pu-depth-intra=1-2,,1-3,1-3,,,1-1
encode_test 264x130 1 1 --pu-depth-inter=1-2,,1-3,1-3,,,1-1

