#!/bin/sh

set -eu
. "${0%/*}/util.sh"

valgrind_test 264x130 10 --source-scan-type=tff -p0 --preset=ultrafast --threads=2 --owf=1 --wpp --gop=0
