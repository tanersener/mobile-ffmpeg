#!/bin/sh

set -eu
. "${0%/*}/util.sh"

valgrind_test 264x130 10 --threads=2 --owf=1 --preset=ultrafast --pu-depth-inter=0-3 --mv-constraint=frametilemargin
valgrind_test 264x130 10 --threads=2 --owf=1 --preset=ultrafast --subme=4 --mv-constraint=frametilemargin
