#!/bin/sh

# Test all-intra coding.

set -eu

. "${0%/*}/util.sh"

common_args='264x130 10 -p1 --threads=2 --owf=1 --no-rdoq --no-deblock --no-sao --no-signhide'
valgrind_test $common_args --rd=1
valgrind_test $common_args --rd=2 --no-transform-skip
