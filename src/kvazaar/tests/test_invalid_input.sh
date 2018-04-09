#!/bin/sh

# Test trying to use invalid input dimensions.

set -eu
. "${0%/*}/util.sh"

encode_test 1x65 1 1
