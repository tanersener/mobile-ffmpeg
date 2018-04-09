#!/bin/sh

# Check for external symbols without kvz_ prefix.

set -eu${BASH+o pipefail}

if nm -go --defined-only ../src/.libs/libkvazaar.a | grep -v ' kvz_'; then
    printf '%s\n' 'Only symbols prefixed with "kvz_" should be exported from libkvazaar.'
    false
fi
