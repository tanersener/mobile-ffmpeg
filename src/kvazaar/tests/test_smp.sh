#!/bin/sh

# Test SMP and AMP blocks.

set -eu
. "${0%/*}/util.sh"

valgrind_test 264x130 4 --threads=2 --owf=1 --wpp --smp
valgrind_test 264x130 4 --threads=2 --owf=1 --wpp --amp
valgrind_test 264x130 4 --threads=2 --owf=1 --wpp --smp --amp
