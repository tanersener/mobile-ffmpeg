#!/bin/sh

# Test OWF, WPP and tiles. There is lots of separate branches of code
# related to owf == 0 and owf != 0, which is why all permutations are
# tried.

set -eu
. "${0%/*}/util.sh"

common_args='-p4 --rd=0 --no-rdoq --no-signhide --subme=0 --deblock --sao --pu-depth-inter=1-3 --pu-depth-intra=2-3'
valgrind_test 264x130 10 $common_args -r1 --owf=1 --threads=0 --no-wpp
valgrind_test 264x130 10 $common_args -r1 --owf=0 --threads=0 --no-wpp
valgrind_test 264x130 10 $common_args -r2 --owf=1 --threads=2 --wpp
valgrind_test 264x130 10 $common_args -r2 --owf=0 --threads=2 --no-wpp
valgrind_test 264x130 10 $common_args -r2 --owf=1 --threads=2 --tiles-height-split=u2 --no-wpp
valgrind_test 264x130 10 $common_args -r2 --owf=0 --threads=2 --tiles-height-split=u2 --no-wpp
valgrind_test 512x512  3 $common_args -r2 --owf=1 --threads=2 --tiles=2x2 --no-wpp
valgrind_test 512x512  3 $common_args -r2 --owf=0 --threads=2 --tiles=2x2 --no-wpp
if [ ! -z ${GITLAB_CI+x} ];then valgrind_test 512x512 30 $common_args -r2 --owf=0 --threads=2 --tiles=2x2 --no-wpp --bipred; fi
