#!/bin/sh

set -eu
. "${0%/*}/util.sh"

valgrind_test 264x130 10 --bitrate=500000 -p0 -r1 --owf=1 --threads=2 --rd=0 --no-rdoq --no-deblock --no-sao --no-signhide --subme=0 --pu-depth-inter=1-3 --pu-depth-intra=2-3
