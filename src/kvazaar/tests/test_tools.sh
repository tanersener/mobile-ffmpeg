#!/bin/sh

# Test RDOQ, SAO, deblock and signhide and subme.

set -eu
. "${0%/*}/util.sh"

common_args='264x130 10 -p0 -r1 --threads=2 --wpp --owf=1 --rd=0'

valgrind_test $common_args --no-rdoq --no-deblock --no-sao --no-signhide --subme=1 --pu-depth-intra=2-3
valgrind_test $common_args --no-rdoq --no-signhide --subme=0
valgrind_test $common_args --rdoq --no-deblock --no-sao --subme=0
