#!/usr/bin/env bash

set -eux

BASE_DIR=$(cd $(dirname $0)/.. && pwd)

exec docker run --rm=true -v $BASE_DIR:/builds/chromaprint -e OS=$OS -e ARCH=$ARCH \
    docker.oxygene.sk/acoustid/chromaprint-build \
    /builds/chromaprint/package/build.sh
