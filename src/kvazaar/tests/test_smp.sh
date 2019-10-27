#!/bin/sh

# Test SMP and AMP blocks.

set -eu
. "${0%/*}/util.sh"

valgrind_test 264x130 4 --threads=2 --owf=1 --wpp --smp
valgrind_test 264x130 4 --threads=2 --owf=1 --wpp --amp
valgrind_test 264x130 4 --threads=2 --owf=1 --wpp --smp --amp
if [ ! -z ${GITLAB_CI+x} ];then valgrind_test 264x130 16 --gop=8 --threads=2 --owf=1 --wpp --smp --amp --bipred; fi
