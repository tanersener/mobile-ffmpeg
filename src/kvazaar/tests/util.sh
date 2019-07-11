#!/bin/sh

# Helper functions for test scripts.

set -eu${BASH+o pipefail}

# Temporary files for encoder input and output.
yuvfile="$(mktemp)"
hevcfile="$(mktemp)"

cleanup() {
    rm -rf "${yuvfile}" "${hevcfile}"
}
trap cleanup EXIT

print_and_run() {
    printf '\n\n$ %s\n' "$*"
    "$@"
}

prepare() {
    cleanup
    print_and_run \
        ffmpeg -f lavfi -i "mandelbrot=size=${1}" \
            -vframes "${2}" -pix_fmt yuv420p -f yuv4mpegpipe \
            "${yuvfile}"
}

valgrind_test() {
    dimensions="$1"
    shift
    frames="$1"
    shift

    prepare "${dimensions}" "${frames}"

    # If $KVZ_TEST_VALGRIND is defined and equal to "1", run the test with
    # valgrind. Otherwise, run without valgrind.
    if [ "${KVZ_TEST_VALGRIND:-0}" = '1' ]; then
        valgrind='valgrind --leak-check=full --error-exitcode=1 --'
    else
        valgrind=''
    fi

    # No quotes for $valgrind because it expands to multiple (or zero)
    # arguments.
    print_and_run \
        libtool execute $valgrind \
            ../src/kvazaar -i "${yuvfile}" "--input-res=${dimensions}" -o "${hevcfile}" "$@"

    print_and_run \
        TAppDecoderStatic -b "${hevcfile}"

    cleanup
}

encode_test() {
    dimensions="$1"
    shift
    frames="$1"
    shift
    expected_status="$1"
    shift

    prepare "${dimensions}" "${frames}"

    set +e
    print_and_run \
        libtool execute \
            ../src/kvazaar -i "${yuvfile}" "--input-res=${dimensions}" -o "${hevcfile}" "$@"
    actual_status="$?"
    set -e
    [ ${actual_status} -eq ${expected_status} ]
}
