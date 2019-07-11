#!/bin/sh

# Test GOP, with and without OWF.

set -eu
. "${0%/*}/util.sh"

common_args='-p0 --threads=2 --wpp --rd=0 --no-rdoq --no-deblock --no-sao --no-signhide --subme=0 --pu-depth-inter=1-3 --pu-depth-intra=2-3'
valgrind_test 264x130 10 $common_args --gop=8 -p0 --owf=1
valgrind_test 264x130 10 $common_args --gop=8 -p0 --owf=4
valgrind_test 264x130 20 $common_args --gop=8 -p16 --owf=0
valgrind_test 264x130 10 $common_args --gop=8 -p1 --owf=4
valgrind_test 264x130 10 $common_args --gop=lp-g4d3t1 -p5 --owf=4
valgrind_test 264x130 10 $common_args --gop=8 -p8 --owf=4 --no-open-gop
valgrind_test 264x130 30 $common_args --gop=8 -p16 --owf=16
# Do more extensive tests in a private gitlab CI runner
if [ ! -z ${GITLAB_CI+x} ];then valgrind_test 264x130 20 $common_args --gop=8 -p8 --owf=0 --no-open-gop; fi
if [ ! -z ${GITLAB_CI+x} ];then valgrind_test 264x130 40 $common_args --gop=8 -p32 --owf=4 --no-open-gop; fi
if [ ! -z ${GITLAB_CI+x} ];then valgrind_test 264x130 70 $common_args --gop=8 -p64 --owf=4 --no-open-gop; fi
if [ ! -z ${GITLAB_CI+x} ];then valgrind_test 264x130 50 $common_args --gop=8 -p40 --owf=4 --no-open-gop; fi
if [ ! -z ${GITLAB_CI+x} ];then valgrind_test 264x130 10 $common_args --gop=8 -p8 --owf=0 --no-open-gop --bipred; fi
